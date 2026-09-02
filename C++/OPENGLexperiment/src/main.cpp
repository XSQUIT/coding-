#include "config.h"
#include "triangle_mesh.h"

unsigned int make_module(const std::string& filepath, unsigned int module_type);

unsigned int make_shader(const std::string& vertex_filepath, const std::string fragment_filepath);

int main() 
{
    GLFWwindow* window; 

    if (!glfwInit())
    {
        std::cout << "GLFW not Initialized" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(640, 480, "OpenGL experiment", NULL, NULL);

    if (!window) 
    {
        std::cout<<"Window not created"<<std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }
    
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    TriangleMesh* triangle = new TriangleMesh();
    TriangleMesh* triangle2 = new TriangleMesh();

    unsigned int shader = make_shader(
        "shaders/vertex.vert",
        "shaders/fragment.frag"
    );

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        triangle->draw();
        triangle2->draw();

        glfwSwapBuffers(window);
    }

    glDeleteProgram(shader);
    delete triangle;
    delete triangle2;
    glfwTerminate();

    return 0;
}    

unsigned int make_module(const std::string& filepath, unsigned int module_type) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cout << "ERROR: Failed to open shader file at: " << filepath << std::endl;
        return 0;
    }

    std::stringstream bufferedLines;
    std::string line;

    while (std::getline(file, line)) {
        bufferedLines << line << "\n";
    }
    std::string shaderSource = bufferedLines.str();
    const char* shaderSrc = shaderSource.c_str();

    unsigned int shaderModule = glCreateShader(module_type);
    glShaderSource(shaderModule, 1, &shaderSrc, NULL);
    glCompileShader(shaderModule);

    int success;
    glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
    if (!success) {
        char errorlog[1024];
        glGetShaderInfoLog(shaderModule, 1024, NULL, errorlog);
        std::cout << "Shader module compilation error:\n" << errorlog << std::endl;
    }
    return shaderModule;
}

unsigned int make_shader(const std::string& vertex_filepath, const std::string fragment_filepath) {

    std::vector<unsigned int> modules;
    modules.push_back(make_module(vertex_filepath, GL_VERTEX_SHADER));
    modules.push_back(make_module(fragment_filepath, GL_FRAGMENT_SHADER));

    unsigned int shader = glCreateProgram();
    for (unsigned int shaderModule : modules) {
        glAttachShader(shader, shaderModule);
    }
    glLinkProgram(shader);

    int success;
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        char errorlog[1024];
        glGetProgramInfoLog(shader, 1024, NULL, errorlog);
        std::cout << "Shader linking error:\n" << errorlog << std::endl;
    }
    for (unsigned int shaderModule : modules) {
        glDeleteShader(shaderModule);
    }

    return shader;
}
