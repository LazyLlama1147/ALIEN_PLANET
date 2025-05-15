#include <cmath>
#include <string>
#include <random>
#include <iostream>
#include <math.h>
#include <cstdlib>

#include "glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "shader.h"
#include "camera.h"
#include "perlin.h"

const GLint WIDTH = 1920, HEIGHT = 1080;

struct plant {
    std::string type;
    float xpos;
    float ypos;
    float zpos;
    int xOffset;
    int yOffset;
    
    plant(std::string _type, float _xpos, float _ypos, float _zpos, int _xOffset, int _yOffset) {
        type = _type;
        xpos = _xpos;
        ypos = _ypos;
        zpos = _zpos;
        xOffset = _xOffset;
        yOffset = _yOffset;
    }
};

// ALL of the Functions
int init();
void processInput(GLFWwindow *window, Shader &shader);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, int &nIndices, std::vector<GLuint> &tree_chunks, std::vector<GLuint> &flower_chunks);

std::vector<int> generate_indices();
std::vector<float> generate_noise_map(int xOffset, int yOffset);
std::vector<float> generate_vertices(const std::vector<float> &noise_map);
std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices);
std::vector<float> generate_biome(const std::vector<float> &vertices, std::vector<plant> &plants, int xOffset, int yOffset);
void generate_map_chunk(GLuint &VAO, int xOffset, int yOffset, std::vector<plant> &plants);

void load_model(GLuint &VAO, std::string filename);
void setup_instancing(GLuint &VAO, std::vector<GLuint> &plant_chunk, std::string plant_type, std::vector<plant> &plants, std::string filename);

GLFWwindow *window;

float WATER_HEIGHT = 0.1;
int chunk_render_distance = 3;
int xMapChunks = 10;
int yMapChunks = 10;
int chunkWidth = 127;
int chunkHeight = 127;
int gridPosX = 0;
int gridPosY = 0;
float originX = (chunkWidth  * xMapChunks) / 2 - chunkWidth / 2;
float originY = (chunkHeight * yMapChunks) / 2 - chunkHeight / 2;


int octaves = 5;
// Vertical scaling
float meshHeight = 32;
// Horizontal scaling
float noiseScale = 64; 
float persistence = 0.5;
float lacunarity = 2;

float MODEL_SCALE = 3;
float MODEL_BRIGHTNESS = 6;

// OPTIONAL FPS 
double lastTime = glfwGetTime();
int nbFrames = 0;

// MAIN CAMERA CONTROLLER
CameraController camera(glm::vec3(originX, 20.0f, originY));
bool firstMouse = true;
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame;




int main() {

    // CONTROLS AKA MAP LEGEND KEY
    std::cout << "=== CONTROLS LEGEND ===" << std::endl;
    std::cout << "W / A / S / D  : Move camera" << std::endl;
    std::cout << "Mouse          : Look around" << std::endl;
    std::cout << "Scroll Wheel   : Zoom in/out" << std::endl;
    std::cout << "F              : Wireframe mode" << std::endl;
    std::cout << "G              : Smooth shading" << std::endl;
    std::cout << "H              : Flat shading" << std::endl;
    std::cout << "Esc            : Exit program" << std::endl;
    std::cout << "=========================" << std::endl;  

    // VARIABLES INITIZLAED
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 projection;
    std::vector<plant> plants;

    //  GLFW & GLAD
    if (init() != 0)
        return -1;
    
    Shader objectShader("objectShader.vert", "objectShader.frag");
    
    // SET TO FLAT MODE
    objectShader.use();
    objectShader.setBool("isFlat", true);
    
    // Lighting driectons 
    objectShader.setVec3("light.ambient", 0.2, 0.2, 0.2);
    objectShader.setVec3("light.diffuse", 0.3, 0.3, 0.3);
    objectShader.setVec3("light.specular", 1.0, 1.0, 1.0);
    objectShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
    
    std::vector<GLuint> map_chunks(xMapChunks * yMapChunks);
    
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            generate_map_chunk(map_chunks[x + y*xMapChunks], x, y, plants);
        }
    
    int nIndices = chunkWidth * chunkHeight * 6;
    
    GLuint treeVAO, flowerVAO;
    std::vector<GLuint> tree_chunks(xMapChunks * yMapChunks);
    std::vector<GLuint> flower_chunks(xMapChunks * yMapChunks);
    
    setup_instancing(treeVAO, tree_chunks, "tree", plants, "CommonTree_1.obj");
    setup_instancing(flowerVAO, flower_chunks, "flower", plants, "Flowers.obj");
    
    while (!glfwWindowShouldClose(window)) {
        objectShader.use();
        projection = glm::perspective(glm::radians(camera.zoom), (float)WIDTH / (float)HEIGHT, 0.1f, (float)chunkWidth * (chunk_render_distance - 1.2f));
        view = camera.getViewMatrix();
        objectShader.setMat4("u_projection", projection);
        objectShader.setMat4("u_view", view);
        objectShader.setVec3("u_viewPos", camera.position);
        
        render(map_chunks, objectShader, view, model, projection, nIndices, tree_chunks, flower_chunks);
    }
    
    for (int i = 0; i < map_chunks.size(); i++) {
        glDeleteVertexArrays(1, &map_chunks[i]);
        glDeleteVertexArrays(1, &tree_chunks[i]);
        glDeleteVertexArrays(1, &flower_chunks[i]);
    }
    
    glfwTerminate();
    
    return 0;
}

void setup_instancing(GLuint &VAO, std::vector<GLuint> &plant_chunk, std::string plant_type, std::vector<plant> &plants, std::string filename) {
    std::vector<std::vector<float>> chunkInstances;
    chunkInstances.resize(xMapChunks * yMapChunks);

    for (int i = 0; i < plants.size(); i++) {
        float xPos = plants[i].xpos / MODEL_SCALE;
        float yPos = plants[i].ypos / MODEL_SCALE;
        float zPos = plants[i].zpos / MODEL_SCALE;
        int pos = plants[i].xOffset + plants[i].yOffset*xMapChunks;
        
        if (plants[i].type == plant_type) {
            chunkInstances[pos].push_back(xPos);
            chunkInstances[pos].push_back(yPos);
            chunkInstances[pos].push_back(zPos);
        }
    }
    
    GLuint instancesVBO[xMapChunks * yMapChunks];
    glGenBuffers(xMapChunks * yMapChunks, instancesVBO);
    
    for (int y = 0; y < yMapChunks; y++) {
        for (int x = 0; x < xMapChunks; x++) {
            int pos = x + y*xMapChunks;
            load_model(plant_chunk[pos], filename);
            
            
            glBindVertexArray(plant_chunk[pos]);
            glBindBuffer(GL_ARRAY_BUFFER, instancesVBO[pos]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * chunkInstances[pos].size(), &chunkInstances[pos][0], GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            
            glVertexAttribDivisor(3, 1);
        }
    }
}

void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, int &nIndices, std::vector<GLuint> &tree_chunks, std::vector<GLuint> &flower_chunks) {
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(window, shader);
    
    glClearColor(0.53, 0.81, 0.92, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Measures number of map chunks 
    gridPosX = (int)(camera.position.x - originX) / chunkWidth + xMapChunks / 2;
    gridPosY = (int)(camera.position.z - originY) / chunkHeight + yMapChunks / 2;
    
    // Render chunks
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            // Only render within render distance
            if (std::abs(gridPosX - x) <= chunk_render_distance && (y - gridPosY) <= chunk_render_distance) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
                shader.setMat4("u_model", model);
                
                // TERRAIN
                glBindVertexArray(map_chunks[x + y*xMapChunks]);
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
                

                
                // PLANT CHUNKS
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
                model = glm::scale(model, glm::vec3(MODEL_SCALE));
                shader.setMat4("u_model", model);

                glEnable(GL_CULL_FACE);
                glBindVertexArray(flower_chunks[x + y*xMapChunks]);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 1300, 16);

                glBindVertexArray(tree_chunks[x + y*xMapChunks]);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 8664, 8);
                glDisable(GL_CULL_FACE);
            }
        }
    
    /* OPTIONAL FPS
    // Measure speed in ms per frame
    double currentTime = glfwGetTime();
    nbFrames++;
    // If last prinf() was more than 1 sec ago printf and reset timer
    if (currentTime - lastTime >= 1.0 ){
        printf("%f ms/frame\n", 1000.0/double(nbFrames));
        nbFrames = 0;
        lastTime += 1.0;
    }
    
    */

    glfwPollEvents();
    glfwSwapBuffers(window);
}

void load_model(GLuint &VAO, std::string filename) {
    std::vector<float> vertices;
    std::vector<int> indices;
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    } else if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertices.push_back(attrib.vertices[3*idx.vertex_index+0]);
                vertices.push_back(attrib.vertices[3*idx.vertex_index+1]);
                vertices.push_back(attrib.vertices[3*idx.vertex_index+2]);
                vertices.push_back(attrib.normals[3*idx.normal_index+0]);
                vertices.push_back(attrib.normals[3*idx.normal_index+1]);
                vertices.push_back(attrib.normals[3*idx.normal_index+2]);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[0] * MODEL_BRIGHTNESS);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[1] * MODEL_BRIGHTNESS);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[2] * MODEL_BRIGHTNESS);
            }
            index_offset += fv;
        }
    }
    GLuint VBO, EBO;
    
    // Create 
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    
    // Bind vertices for VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    // vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // vertex normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // vertex color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void generate_map_chunk(GLuint &VAO, int xOffset, int yOffset, std::vector<plant> &plants) {
    std::vector<int> indices;
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    
    // GENERATE THE MAIN MAP
    indices = generate_indices();
    noise_map = generate_noise_map(xOffset, yOffset);
    vertices = generate_vertices(noise_map);
    normals = generate_normals(indices, vertices);
    colors = generate_biome(vertices, plants, xOffset, yOffset);
    
    GLuint VBO[3], EBO;
    
    glGenBuffers(3, VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    
    // vertices to VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    // THE ELEMENT BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
    
    // vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // vertices to VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
    
    // vertex normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
}

glm::vec3 get_color(int r, int g, int b) {
    return glm::vec3(r/255.0, g/255.0, b/255.0);
}

std::vector<float> generate_noise_map(int offsetX, int offsetY) {
    std::vector<float> noiseValues;
    std::vector<float> normalizedNoiseValues;
    std::vector<int> p = buildPermutationTable();
    
    float amp  = 1;
    float freq = 1;
    float maxPossibleHeight = 0;
    
    for (int i = 0; i < octaves; i++) {
        maxPossibleHeight += amp;
        amp *= persistence;
    }

    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            amp  = 1;
            freq = 1;
            float noiseHeight = 0;
            for (int i = 0; i < octaves; i++) {
                float xSample = (x + offsetX * (chunkWidth-1))  / noiseScale * freq;
                float ySample = (y + offsetY * (chunkHeight-1)) / noiseScale * freq;
                
                float perlinValue = alienNoise3D(xSample, ySample, p);
                noiseHeight += perlinValue * amp;
                
                amp  *= persistence;
                freq *= lacunarity;
            }
            noiseValues.push_back(noiseHeight);
        }
    }
    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            normalizedNoiseValues.push_back((noiseValues[x + y*chunkWidth] + 1) / maxPossibleHeight);
        }
    }
    return normalizedNoiseValues;
}

struct terrainColor {
    terrainColor(float _height, glm::vec3 _color) {
        height = _height;
        color = _color;
    };
    float height;
    glm::vec3 color;
};

std::vector<float> generate_biome(const std::vector<float> &vertices, std::vector<plant> &plants, int xOffset, int yOffset) {
    std::vector<float> colors;
    std::vector<terrainColor> biomeColors;
    glm::vec3 color = get_color(255, 255, 255);
    
    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, get_color(60,  95, 190)));   // Deep water
    biomeColors.push_back(terrainColor(WATER_HEIGHT,        get_color(60, 100, 190)));  // Shallow water
    biomeColors.push_back(terrainColor(0.15, get_color(190, 240, 150))); // Pale alien sand

    biomeColors.push_back(terrainColor(0.30, get_color(255, 105, 180)));                // Pink Grass 1
    biomeColors.push_back(terrainColor(0.40, get_color(255, 182, 193)));                // Pink Grass 2
    biomeColors.push_back(terrainColor(0.50, get_color(120, 72, 160)));   // Rock 1: muted violet-gray
    biomeColors.push_back(terrainColor(0.80, get_color(100, 85, 140)));   // Rock 2: dusty blue-purple
    biomeColors.push_back(terrainColor(1.00, get_color(255, 255, 255)));                // Snow
    
    std::string plantType;
    
    // Determine which color to assign each vertex by its y-coord
    for (int i = 1; i < vertices.size(); i += 3) {
        for (int j = 0; j < biomeColors.size(); j++) {
            if (vertices[i] <= biomeColors[j].height * meshHeight) {
                color = biomeColors[j].color;
                if (j == 3) {
                    if (rand() % 1000 < 5) {
                        if (rand() % 100 < 70) {
                            plantType = "flower";
                        } else {
                            plantType = "tree";
                        }
                        plants.push_back(plant{plantType, vertices[i-1], vertices[i], vertices[i+1], xOffset, yOffset});
                    }
                }
                break;
            }
        }
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    return colors;
}

std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices) {
    int pos;
    glm::vec3 normal;
    std::vector<float> normals;
    std::vector<glm::vec3> verts;
    for (int i = 0; i < indices.size(); i += 3) {
            for (int j = 0; j < 3; j++) {
            pos = indices[i+j]*3;
            verts.push_back(glm::vec3(vertices[pos], vertices[pos+1], vertices[pos+2]));
        }
        
        glm::vec3 U = verts[i+1] - verts[i];
        glm::vec3 V = verts[i+2] - verts[i];
        
        normal = glm::normalize(-glm::cross(U, V));
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }
    
    return normals;
}

std::vector<float> generate_vertices(const std::vector<float> &noise_map) {
    std::vector<float> v;
    for (int y = 0; y < chunkHeight + 1; y++)
        for (int x = 0; x < chunkWidth; x++) {
            v.push_back(x);
            float easedNoise = std::pow(noise_map[x + y*chunkWidth] * 1.1, 3);
            v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
            v.push_back(y);
        }
    return v;
}

std::vector<int> generate_indices() {
    std::vector<int> indices;
    
    for (int y = 0; y < chunkHeight; y++)
        for (int x = 0; x < chunkWidth; x++) {
            int pos = x + y*chunkWidth;
            
            if (x == chunkWidth - 1 || y == chunkHeight - 1) {
                continue;
            } else {
                // Top left
                indices.push_back(pos + chunkWidth);
                indices.push_back(pos);
                indices.push_back(pos + chunkWidth + 1);
                // Bottom right 
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + chunkWidth);
                indices.push_back(pos);
            }
        }

    return indices;
}

int init() {
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "Terrain Generator", nullptr, nullptr);
    
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    return 0;
}

void processInput(GLFWwindow *window, Shader &shader) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // wireframe mode
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // flat mode
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        shader.use();
        shader.setBool("isFlat", false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        shader.use();
        shader.setBool("isFlat", true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.handleKeyboard(MOVE_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.handleKeyboard(MOVE_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.handleKeyboard(MOVE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.handleKeyboard(MOVE_RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.handleMouse(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.handleMouseScroll(yoffset);
}
