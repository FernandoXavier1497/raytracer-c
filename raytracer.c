#include "raylib.h"
#include "raymath.h"   
#include <math.h>
#include <float.h>     // Para FLT_MAX

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define AMBIENT       0.12f   // Luz ambiente: evita o lado escuro ficar preto puro

// Definições de Estruturas
typedef struct {
    Color color;
    float specularStrength;
    float shininess;
} SurfaceMaterial;

// O material agora vive dentro da esfera (sem array paralelo para desalinhar)
typedef struct {
    Vector3 position;
    float radius;
    SurfaceMaterial material;
} Sphere;


static unsigned char pixels[SCREEN_HEIGHT][SCREEN_WIDTH][4];

static unsigned char ClampToByte(float v) {
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}

int main(void) {
    // --- Inicialização ---
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Software Ray Tracer");
    SetTargetFPS(60);

    // --- Setup da Cena ---

    // Câmera 3D
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 5.0f }; // Olhando para frente (Z+)
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Direção em que a luz VIAJA (Sol)
    Vector3 lightDir = Vector3Normalize((Vector3){ -0.5f, -1.0f, -0.5f });

    // Esferas na cena (posição, raio, material{cor, specularStrength, shininess})
    const int sphereCount = 6;
    Sphere spheres[6] = {
        { { 0.0f,  0.0f, 5.0f }, 2.0f, { RED,    0.5f, 32.0f } },  // Grande no meio
        { { -3.0f, 1.0f, 7.0f }, 1.0f, { BLUE,   0.2f, 16.0f } },  // Azul à esquerda/fundo
        { { 3.0f, -1.0f, 6.0f }, 1.5f, { BROWN,  0.0f,  1.0f } },  // Marrom à direita/baixo (sem brilho)
        { { 0.0f,  2.5f, 8.0f }, 1.0f, { WHITE,  0.8f, 64.0f } },  // Branca topo/fundo (muito brilhante)
        { { -2.0f,-2.0f, 4.0f }, 0.8f, { GREEN,  0.3f, 24.0f } },  // Verde perto/esquerda
        { { 2.0f,  2.0f, 9.0f }, 1.2f, { PURPLE, 0.6f, 48.0f } }   // Roxa longe/direita
    };

    // --- Renderização (a cena é estática, então renderizamos uma única vez) ---
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {

            // 1. Raio da câmera passando pelo pixel (x, y)
            Ray ray = GetScreenToWorldRay((Vector2){ (float)x, (float)y }, camera);

            // 2. Colisão com todas as esferas (guarda a mais próxima)
            bool hit = false;
            float nearestDistance = FLT_MAX;
            RayCollision nearestCollision = { 0 };   // CORREÇÃO 4: inicializada
            int nearestSphereIndex = -1;

            for (int i = 0; i < sphereCount; i++) {
                RayCollision collision = GetRayCollisionSphere(ray, spheres[i].position, spheres[i].radius);

                if (collision.hit && collision.distance < nearestDistance) {
                    hit = true;
                    nearestDistance = collision.distance;
                    nearestCollision = collision;
                    nearestSphereIndex = i;
                }
            }

            // 3. Cor do pixel
            if (hit) {
                SurfaceMaterial m = spheres[nearestSphereIndex].material;
                Vector3 normal = nearestCollision.normal;

                // A. Difusa (Lambert)
                float diffuse = fmaxf(Vector3DotProduct(normal, Vector3Negate(lightDir)), 0.0f);

                // B. Especular (Phong)
                float specular = 0.0f;
                if (diffuse > 0.0f) {   // sem luz na superfície, sem brilho
                    Vector3 viewDir = Vector3Normalize(Vector3Subtract(camera.position, nearestCollision.point));

                    
                    Vector3 reflectDir = Vector3Reflect(lightDir, normal);

                    float specAngle = fmaxf(Vector3DotProduct(viewDir, reflectDir), 0.0f);
                    specular = powf(specAngle, m.shininess) * m.specularStrength;
                }

                // Cor base escalada por ambiente + difusa; o brilho especular é somado em BRANCO
                float lit = AMBIENT + diffuse * (1.0f - AMBIENT);
                float spec255 = specular * 255.0f;

                pixels[y][x][0] = ClampToByte(m.color.r * lit + spec255);
                pixels[y][x][1] = ClampToByte(m.color.g * lit + spec255);
                pixels[y][x][2] = ClampToByte(m.color.b * lit + spec255);
                pixels[y][x][3] = 255;
            } else {
                // Fundo preto
                pixels[y][x][0] = 0;
                pixels[y][x][1] = 0;
                pixels[y][x][2] = 0;
                pixels[y][x][3] = 255;
            }
        }
    }

    // Textura criada UMA vez (antes era criada/descartada a cada frame)
    Image img = {
        .data = pixels,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    Texture2D tex = LoadTextureFromImage(img);

    // --- Loop principal: só desenha a textura pronta ---
    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexture(tex, 0, 0, WHITE);
            DrawText("Software Ray Tracing", 10, 10, 20, RAYWHITE);
            DrawFPS(10, 35);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
