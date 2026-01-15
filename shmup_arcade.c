// Shoot 'em up arcade (single-file, vector art, no music)
// Compile (Windows MinGW): gcc shmup_arcade.c -o shmup_arcade.exe -std=c99 -O2 -lmingw32 -lSDL2main -lSDL2
// Compile (Linux/macOS): gcc shmup_arcade.c -o shmup_arcade `sdl2-config --cflags --libs` -std=c99 -O2

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#define W 800
#define H 600

#define MAX_BULLETS 128
#define MAX_ENEMIES 64
#define MAX_PARTICLES 200
#define MAX_BOSS_BULLETS 256

typedef struct { float x,y,vx,vy; int alive; } Obj;

static Obj bullets[MAX_BULLETS];
static Obj enemies[MAX_ENEMIES];
static Obj particles[MAX_PARTICLES];
static Obj bossBullets[MAX_BOSS_BULLETS];

static float px, py;
static int p_hp = 3;
static int score = 0;
static int lives = 3;
static int fireCooldown = 0;
static int enemySpawnTimer = 0;
static int gameOver = 0;
static int stageTimer = 0;

// Boss
static int bossAlive = 0;
static float bossX=0, bossY=0, bossVX=0, bossHP=0;
static int bossPhase = 0;
static int bossShootTimer = 0;
static int bossEntranceTimer = 0;

// Utility
static float clamp(float v, float a, float b){ if(v<a) return a; if(v>b) return b; return v; }
static float rndf(){ return (rand()/(float)RAND_MAX); }
static float dist2(float x1,float y1,float x2,float y2){ float dx=x1-x2, dy=y1-y2; return dx*dx+dy*dy; }

void spawnParticle(float x, float y, int n){
    for(int i=0;i<MAX_PARTICLES && n>0;i++){
        if(!particles[i].alive){
            particles[i].alive=1;
            float a = rndf()*M_PI*2;
            float s = 1.0f + rndf()*2.5f;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = cosf(a)*s;
            particles[i].vy = sinf(a)*s;
            n--;
        }
    }
}

void spawnEnemy(){
    for(int i=0;i<MAX_ENEMIES;i++){
        if(!enemies[i].alive){
            enemies[i].alive=1;
            enemies[i].x = 40 + rand()%(W-80);
            enemies[i].y = -10 - (rand()%200);
            enemies[i].vx = (rndf()-0.5f)*1.2f;
            enemies[i].vy = 1.0f + rndf()*1.2f;
            break;
        }
    }
}

void fireBullet(float x,float y,float vx,float vy){
    for(int i=0;i<MAX_BULLETS;i++){
        if(!bullets[i].alive){
            bullets[i].alive=1;
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].vx = vx;
            bullets[i].vy = vy;
            break;
        }
    }
}

void fireBossBullet(float x,float y,float vx,float vy){
    for(int i=0;i<MAX_BOSS_BULLETS;i++){
        if(!bossBullets[i].alive){
            bossBullets[i].alive=1;
            bossBullets[i].x = x;
            bossBullets[i].y = y;
            bossBullets[i].vx = vx;
            bossBullets[i].vy = vy;
            break;
        }
    }
}

void resetLevel(){
    for(int i=0;i<MAX_BULLETS;i++) bullets[i].alive=0;
    for(int i=0;i<MAX_ENEMIES;i++) enemies[i].alive=0;
    for(int i=0;i<MAX_PARTICLES;i++) particles[i].alive=0;
    for(int i=0;i<MAX_BOSS_BULLETS;i++) bossBullets[i].alive=0;
    px = W/2; py = H - 80;
    p_hp = 3;
    fireCooldown = 0;
    enemySpawnTimer = 0;
    bossAlive = 0;
    bossEntranceTimer = 0;
    bossShootTimer = 0;
    bossHP = 0;
    bossPhase = 0;
    stageTimer = 0;
    score = 0;
    lives = 3;
    gameOver = 0;
}

void spawnBoss(){
    bossAlive = 1;
    bossX = W/2;
    bossY = -120;
    bossVX = 0;
    bossHP = 300 + rand()%150; // HP variable
    bossPhase = 0;
    bossEntranceTimer = 0;
    bossShootTimer = 0;
}

void updatePhysics(){
    // player input handled externally
    // bullets
    for(int i=0;i<MAX_BULLETS;i++){
        if(bullets[i].alive){
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;
            // offscreen
            if(bullets[i].y < -20 || bullets[i].y>H+20 || bullets[i].x < -20 || bullets[i].x > W+20)
                bullets[i].alive = 0;
        }
    }
    // enemies
    for(int i=0;i<MAX_ENEMIES;i++){
        if(enemies[i].alive){
            enemies[i].x += enemies[i].vx;
            enemies[i].y += enemies[i].vy;
            // bounce horizontally
            if(enemies[i].x < 10 || enemies[i].x > W-10) enemies[i].vx *= -1;
            // off bottom
            if(enemies[i].y > H+30) enemies[i].alive = 0;
            // collision with player
            if(dist2(enemies[i].x,enemies[i].y,px,py) < 20*20){
                enemies[i].alive = 0;
                spawnParticle(px,py,18);
                p_hp--;
                if(p_hp<=0){
                    lives--;
                    if(lives>0){
                        p_hp = 3;
                        px = W/2; py = H-80;
                    } else gameOver = 1;
                }
            }
            // collision with bullets
            for(int b=0;b<MAX_BULLETS;b++) if(bullets[b].alive){
                if(dist2(enemies[i].x,enemies[i].y,bullets[b].x,bullets[b].y) < 16*16){
                    bullets[b].alive = 0;
                    enemies[i].alive = 0;
                    spawnParticle(enemies[i].x, enemies[i].y, 12 + rand()%8);
                    score += 10;
                }
            }
        }
    }
    // particles
    for(int i=0;i<MAX_PARTICLES;i++){
        if(particles[i].alive){
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vx *= 0.98f;
            particles[i].vy *= 0.98f;
            // fade by moving off
            if(particles[i].x < -30 || particles[i].x > W+30 || particles[i].y < -30 || particles[i].y > H+30)
                particles[i].alive = 0;
        }
    }
    // boss logic
    if(bossAlive){
        bossEntranceTimer++;
        if(bossEntranceTimer < 120){
            // ease into screen
            bossY += 1.5f;
        } else {
            // movement
            bossX += bossVX;
            // simple AI: follow player with some inertia
            bossVX += (px - bossX) * 0.0025f;
            bossVX = clamp(bossVX, -3, 3);
            if(bossX < 60) bossX = 60, bossVX = -bossVX;
            if(bossX > W-60) bossX = W-60, bossVX = -bossVX;

            // phases by HP proportion
            float prop = bossHP / 400.0f;
            if(prop < 0.6f) bossPhase = 1;
            if(prop < 0.35f) bossPhase = 2;

            // shooting patterns
            bossShootTimer++;
            if(bossPhase==0){
                if(bossShootTimer % 40 == 0){
                    // straightforward spread
                    for(int a=-2;a<=2;a++){
                        float ang = M_PI/2 + a*0.2f;
                        float spd = 2.5f;
                        fireBossBullet(bossX, bossY+30, cosf(ang)*spd, sinf(ang)*spd);
                    }
                }
            } else if(bossPhase==1){
                if(bossShootTimer % 28 == 0){
                    // aimed burst
                    float dx = px - bossX; float dy = py - bossY;
                    float base = atan2f(dy,dx);
                    for(int a=-3;a<=3;a++){
                        float ang = base + a*0.12f;
                        fireBossBullet(bossX, bossY+30, cosf(ang)*3.2f, sinf(ang)*3.2f);
                    }
                }
                // occasional dive
                if(bossShootTimer % 300 == 0){
                    bossVX += (rndf()-0.5f)*6.0f;
                    bossY += 20 * (rndf()-0.5f);
                }
            } else { // phase 2: faster and spirals
                if(bossShootTimer % 6 == 0){
                    float ang = (bossShootTimer % 360) * 0.06f;
                    float spd = 2.0f + (rndf()*1.6f);
                    fireBossBullet(bossX + cosf(ang)*30, bossY + sinf(ang)*20, cosf(ang)*spd, sinf(ang)*spd);
                }
                if(bossShootTimer % 120 == 0){
                    // radial burst
                    for(int a=0;a<16;a++){
                        float ang = a * (2*M_PI/16.0f) + rndf()*0.2f;
                        fireBossBullet(bossX, bossY+30, cosf(ang)*2.8f, sinf(ang)*2.8f);
                    }
                }
            }
        }
    }

    // boss bullets
    for(int i=0;i<MAX_BOSS_BULLETS;i++){
        if(bossBullets[i].alive){
            bossBullets[i].x += bossBullets[i].vx;
            bossBullets[i].y += bossBullets[i].vy;
            // slow gravity-ish
            bossBullets[i].vy += 0.01f;
            if(bossBullets[i].x < -20 || bossBullets[i].x > W+20 || bossBullets[i].y > H+40 || bossBullets[i].y < -40)
                bossBullets[i].alive = 0;
            // collision with player
            if(dist2(bossBullets[i].x,bossBullets[i].y,px,py) < 12*12){
                bossBullets[i].alive = 0;
                spawnParticle(px,py,18);
                p_hp--;
                if(p_hp<=0){
                    lives--;
                    if(lives>0){
                        p_hp = 3;
                        px = W/2; py = H-80;
                    } else gameOver = 1;
                }
            }
        }
    }

    // collision bullets -> boss
    if(bossAlive && bossEntranceTimer >= 120){
        for(int i=0;i<MAX_BULLETS;i++) if(bullets[i].alive){
            if(dist2(bullets[i].x,bullets[i].y,bossX,bossY) < 48*48){
                bullets[i].alive = 0;
                bossHP -= 4;
                spawnParticle(bullets[i].x, bullets[i].y, 6);
                if(bossHP <= 0){
                    bossAlive = 0;
                    score += 500;
                    spawnParticle(bossX,bossY,80);
                }
            }
        }
    }

    // spawn enemies over time
    enemySpawnTimer++;
    stageTimer++;
    if(!bossAlive && stageTimer > 1200){ // after a while spawn a boss
        spawnBoss();
    }
    if(stageTimer % 20 == 0 && !bossAlive){
        if(rand()%8 < 5) spawnEnemy();
    }

    // simple difficulty ramp: occasionally spawn extra
    if(stageTimer % 600 == 0 && !bossAlive){
        for(int i=0;i<3;i++) spawnEnemy();
    }

    // small cap on velocities, etc
    // clean dead
}

void renderVectorTriangle(SDL_Renderer* r, float cx, float cy, float size, float angle){
    // 3-point ship triangle
    float a1 = angle;
    float a2 = angle + 2.5f;
    float a3 = angle - 2.5f;
    int x1 = (int)(cx + cosf(a1)*size);
    int y1 = (int)(cy + sinf(a1)*size);
    int x2 = (int)(cx + cosf(a2)*size*0.75f);
    int y2 = (int)(cy + sinf(a2)*size*0.75f);
    int x3 = (int)(cx + cosf(a3)*size*0.75f);
    int y3 = (int)(cy + sinf(a3)*size*0.75f);
    SDL_RenderDrawLine(r,x1,y1,x2,y2);
    SDL_RenderDrawLine(r,x2,y2,x3,y3);
    SDL_RenderDrawLine(r,x3,y3,x1,y1);
}

void draw(SDL_Renderer* r){
    // background gradient-ish (simple)
    Uint8 base = 8;
    SDL_SetRenderDrawColor(r, base, base, base+18, 255);
    SDL_RenderClear(r);

    // starfield: quick randomized stars based on score/time (not persistent)
    for(int i=0;i<80;i++){
        int sx = (i*37 + (score*3)) % W;
        int sy = (i*53 + (score*5)) % H;
        SDL_SetRenderDrawColor(r, 20 + (i%3)*10, 20 + (i%5)*5, 40, 255);
        SDL_RenderDrawPoint(r, sx, sy);
    }

    // player - vector triangle with thrust effect
    SDL_SetRenderDrawColor(r, 120, 220, 200, 255);
    renderVectorTriangle(r, px, py, 12, -M_PI/2);

    // player hp bars
    for(int i=0;i<p_hp;i++){
        SDL_Rect hp = {10 + i*18, H-26, 14, 12};
        SDL_SetRenderDrawColor(r, 220, 80, 80, 255);
        SDL_RenderFillRect(r, &hp);
    }

    // bullets
    SDL_SetRenderDrawColor(r, 250, 250, 120, 255);
    for(int i=0;i<MAX_BULLETS;i++) if(bullets[i].alive){
        SDL_Rect b = {(int)bullets[i].x-2, (int)bullets[i].y-6, 4, 10};
        SDL_RenderFillRect(r, &b);
    }

    // enemies - vector squares/diamonds
    for(int i=0;i<MAX_ENEMIES;i++) if(enemies[i].alive){
        int ex = (int)enemies[i].x;
        int ey = (int)enemies[i].y;
        SDL_SetRenderDrawColor(r, 200, 120, 80, 255);
        // diamond
        SDL_RenderDrawLine(r, ex, ey-8, ex+8, ey);
        SDL_RenderDrawLine(r, ex+8, ey, ex, ey+8);
        SDL_RenderDrawLine(r, ex, ey+8, ex-8, ey);
        SDL_RenderDrawLine(r, ex-8, ey, ex, ey-8);
    }

    // particles
    for(int i=0;i<MAX_PARTICLES;i++) if(particles[i].alive){
        SDL_SetRenderDrawColor(r, 255, 200, 120, 255);
        SDL_RenderDrawPoint(r, (int)particles[i].x, (int)particles[i].y);
    }

    // boss
    if(bossAlive){
        // boss body - layered vector shapes
        SDL_SetRenderDrawColor(r, 180, 80, 200, 255);
        int bx = (int)bossX, by = (int)bossY;
        // central hull (circle approximation by diamond + lines)
        for(int rsz = 40; rsz>=20; rsz-=10){
            SDL_SetRenderDrawColor(r, 180-rsz, 80+rsz, 200-rsz, 255);
            // horizontal bar
            SDL_Rect rect = {bx-rsz*2, by-rsz/2, rsz*4, rsz};
            SDL_RenderFillRect(r, &rect);
        }
        // wings
        SDL_SetRenderDrawColor(r, 220, 120, 160, 255);
        SDL_RenderDrawLine(r, bx-70, by+10, bx-30, by-20);
        SDL_RenderDrawLine(r, bx+70, by+10, bx+30, by-20);
        // boss HP bar above
        SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
        SDL_Rect back = {bx - 80, by - 70, 160, 10};
        SDL_RenderFillRect(r, &back);
        if(bossHP>0){
            float prop = clamp(bossHP / 450.0f, 0.0f, 1.0f);
            SDL_SetRenderDrawColor(r, 160, 40, 40, 255);
            SDL_Rect bar = {bx - 80, by - 70, (int)(160 * prop), 10};
            SDL_RenderFillRect(r, &bar);
        }
    }

    // boss bullets
    SDL_SetRenderDrawColor(r, 255, 140, 120, 255);
    for(int i=0;i<MAX_BOSS_BULLETS;i++) if(bossBullets[i].alive){
        SDL_Rect bb = {(int)bossBullets[i].x-3, (int)bossBullets[i].y-3, 6, 6};
        SDL_RenderFillRect(r, &bb);
    }

    // HUD: score and lives
    char buf[64];
    snprintf(buf,64,"Score: %d", score);
    // We don't render text (no SDL_ttf). We'll render approximate digits using rectangles (very tiny)
    // Simple numeric HUD: Score (approx) by blocks
    SDL_SetRenderDrawColor(r, 200, 200, 220, 255);
    for(int i=0;i< (score/10) && i<20;i++){
        SDL_Rect s = {W-120 + i*4, H-20, 3, 10};
        SDL_RenderFillRect(r, &s);
    }
    // Lives as icons
    for(int i=0;i<lives;i++){
        SDL_SetRenderDrawColor(r, 180, 220, 255, 255);
        int lx = W-20 - i*18;
        int ly = 12;
        renderVectorTriangle(r, lx, ly, 6, -M_PI/2);
    }

    // Game Over overlay
    if(gameOver){
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
        SDL_Rect overlay = {0,0,W,H};
        SDL_RenderFillRect(r, &overlay);
        // draw a centered "GAME OVER" using rectangles as letters
        // G
        SDL_SetRenderDrawColor(r, 240, 80, 80, 255);
        int cx = W/2 - 120;
        for(int y=0;y<50;y+=6){
            SDL_Rect seg = {cx, H/2 - 30 + y, 8, 4};
            SDL_RenderFillRect(r, &seg);
        }
        // simple prompt: user can press R to restart
    }

    SDL_RenderPresent(r);
}

int main(int argc, char** argv){
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0){
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("SHMUP Arcade - Single File", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    if(!win){ printf("Window error: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!rend){ printf("Renderer error: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    resetLevel();

    Uint32 lastTick = SDL_GetTicks();
    int shootHeld = 0;
    while(1){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){ goto cleanup; }
            if(e.type == SDL_KEYDOWN){
                if(e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) goto cleanup;
                if(e.key.keysym.scancode == SDL_SCANCODE_R && gameOver){
                    resetLevel(); continue;
                }
            }
        }
        const Uint8* ks = SDL_GetKeyboardState(NULL);
        if(!gameOver){
            // player movement
            if(ks[SDL_SCANCODE_LEFT] || ks[SDL_SCANCODE_A]) px -= 4.0f;
            if(ks[SDL_SCANCODE_RIGHT] || ks[SDL_SCANCODE_D]) px += 4.0f;
            if(ks[SDL_SCANCODE_UP] || ks[SDL_SCANCODE_W]) py -= 4.0f;
            if(ks[SDL_SCANCODE_DOWN] || ks[SDL_SCANCODE_S]) py += 4.0f;
            px = clamp(px, 16, W-16);
            py = clamp(py, 60, H-20);

            // shooting
            if(ks[SDL_SCANCODE_SPACE]){
                if(!shootHeld && fireCooldown <= 0){
                    // primary shot: three bullets (spread)
                    fireBullet(px, py-16, -0.6f, -6.0f);
                    fireBullet(px, py-16, 0.0f, -6.6f);
                    fireBullet(px, py-16, 0.6f, -6.0f);
                    fireCooldown = 8;
                }
                shootHeld = 1;
            } else shootHeld = 0;

            if(fireCooldown>0) fireCooldown--;

            // Update physics and enemies
            updatePhysics();
        } else {
            // gameOver state — wait for R or ESC handled earlier
        }

        // draw
        draw(rend);

        // frame delay basic
        Uint32 now = SDL_GetTicks();
        Uint32 frameTime = now - lastTick;
        if(frameTime < 16) SDL_Delay(16 - frameTime);
        lastTick = SDL_GetTicks();

        // small score accumulation for survival
        if(!gameOver) score += 0; // we give points for kills already
    }

cleanup:
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
