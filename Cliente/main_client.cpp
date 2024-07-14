#include <..\include\SDL2\SDL.h>
#include <GL/glew.h>
#include <iostream>
#include "NetworkClient.h"

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "2323"

SDL_Window* window = nullptr;
SDL_GLContext context;
NetworkClient client(SERVER_ADDRESS, SERVER_PORT);

const int MAX_X = 24;
const int MIN_X = 0;
const int MAX_Y = 18;
const int MIN_Y = 0;

// Variáveis para a câmera
float cameraX = 0.0f;
float cameraY = 0.0f;
float zoomScale = 1.0f;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Erro ao inicializar SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Configuração do contexto OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    // Criação da janela SDL
     window = SDL_CreateWindow("Cliente OpenGL",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED);

    if (!window) {
        std::cerr << "Erro ao criar janela SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Criação do contexto OpenGL
    context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "Erro ao criar contexto OpenGL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Inicialização do GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Erro ao inicializar GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    // Inicialização do cliente de rede
    if (!client.initialize() || !client.connectToServer()) {
        std::cerr << "Falha ao conectar ao servidor." << std::endl;
        return false;
    }

    return true;
}

void updateCamera() {
    // Ajusta a câmera para que o centro da tela seja o ponto focal
    float screenWidth = SDL_GetWindowSurface(window)->w / zoomScale;
    float screenHeight = SDL_GetWindowSurface(window)->h / zoomScale;
    
    cameraX = (client.pos_x * SQUARE_SIZE) - screenWidth / 2;
    cameraY = (client.pos_y * SQUARE_SIZE) - screenHeight / 2;

    // Restringe a câmera para não ultrapassar os limites do mapa
    if (cameraX < 0) {
        cameraX = 0;
    }
    if (cameraY < 0) {
        cameraY = 0;
    }
    if (cameraX > client.mapWidth * SQUARE_SIZE - screenWidth) {
        cameraX = client.mapWidth * SQUARE_SIZE - screenWidth;
    }
    if (cameraY > client.mapHeight * SQUARE_SIZE - screenHeight) {
        cameraY = client.mapHeight * SQUARE_SIZE - screenHeight;
    }
}

void render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Ajusta a matriz de projeção com base na câmera e no zoom
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glOrtho(cameraX, cameraX + windowWidth / zoomScale, cameraY + windowHeight / zoomScale, cameraY, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    // Desenha todos os jogadores
    {
        std::lock_guard<std::mutex> lock(client.playersMutex);
        for (const auto& playerPair : client.players) {
            const auto& player = playerPair.second;
            int squareLeft = player.pos_x * SQUARE_SIZE;
            int squareRight = squareLeft + SQUARE_SIZE;
            int squareTop = player.pos_y * SQUARE_SIZE;
            int squareBottom = squareTop + SQUARE_SIZE;

            glColor3f(player.color[0], player.color[1], player.color[2]);
            glBegin(GL_QUADS);
            glVertex2i(squareLeft, squareTop);
            glVertex2i(squareRight, squareTop);
            glVertex2i(squareRight, squareBottom);
            glVertex2i(squareLeft, squareBottom);
            glEnd();
        }
    }

    SDL_GL_SwapWindow(window);
}

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;
            case SDL_KEYDOWN:
                {
                    std::string message;
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            if (client.pos_y > MIN_Y) {
                                client.pos_y -= 1; // Move para cima
                            }
                            break;
                        case SDLK_s:
                            if (client.pos_y < MAX_Y) {
                                client.pos_y += 1; // Move para baixo
                            }
                            break;
                        case SDLK_a:
                            if (client.pos_x > MIN_X) {
                                client.pos_x -= 1; // Move para a esquerda
                            }
                            break;
                        case SDLK_d:
                            if (client.pos_x < MAX_X) {
                                client.pos_x += 1; // Move para a direita
                            }
                            break;
                        case SDLK_EQUALS: // Tecla '+' para aumentar o zoom
                            zoomScale *= 1.1f;
                            break;
                        case SDLK_MINUS: // Tecla '-' para reduzir o zoom
                            zoomScale /= 1.1f;
                            break;
                        default:
                            break;
                    }

                    // Limita client.pos_x e client.pos_y aos valores permitidos
                    client.pos_x = std::max(MIN_X, std::min(MAX_X, client.pos_x));
                    client.pos_y = std::max(MIN_Y, std::min(MAX_Y, client.pos_y));

                    // Atualiza a posição do jogador local no mapa de jogadores
                    client.updatePlayerPosition(0, client.pos_x, client.pos_y);

                    // Envia a mensagem com a nova posição para o servidor
                    message = std::to_string(client.pos_x) + " " + std::to_string(client.pos_y);
                    if (!message.empty()) {
                        if (!client.sendMessage(message.c_str())) {
                            std::cerr << "Falha ao enviar mensagem para o servidor." << std::endl;
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (!initSDL()) {
        return 1;
    }

    bool quit = false;
    while (!quit) {
        handleInput();
        updateCamera();
        render();
    }

    // Libera recursos SDL e OpenGL
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
