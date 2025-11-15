#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "ui.h"
#include "structs.h"
#include "vendor/stb_image.h"

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/*
 * Константи.
 */
constexpr auto clear_color = glm::vec4(0.45f, 0.55f, 0.60f, 0.90f);

/*
 * Глобални променливи. За удобство.
 */
static std::unordered_map<std::string, int> g_uniform_locations;
static unsigned int g_program = 0;
static glm::mat4 g_model = glm::mat4(1.0f);

static glm::vec3 g_light_pos = glm::vec3(1.0f, 1.0f, 2.0f);
static glm::vec3 g_light_color = glm::vec3(1.0f); /* White light */

// Променливи за движение на робота
float g_move_speed = 0.1f;  // Скорост на движение напред/назад/странично
float g_rotation_speed = 2.0f;  // Скорост на завъртане (в градуси)

/*
 * Предварителни декларации на функции.
 */
static void set_model(unsigned int);
static void set_view(unsigned int);
static void set_projection(unsigned int);
static void set_light_pos(unsigned int);
static void set_light_color(unsigned int);
static void draw_cuboid(const glm::vec3& size);
static void draw_robot();

/*
 * Проверка за грешки в OpenGL.
 * Извежда съобщение в конзолата ако има грешка.
 */
static unsigned int gl_print_error(void)
{
    unsigned int error = glGetError();
    if (error == 0)
        std::cout << "No GL errors." << std::endl;
    else
        std::cerr << "GL Error: " << error << std::endl;
    return error;
}

/*
 * Callback функция за грешки в GLFW прозореца.
 * Извежда съобщение за грешка в конзолата.
 */
static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error: " << error << " " << description << std::endl;
}

/*
 * Callback функция за натискане на клавиши.
 * Обработва потребителския вход и управлява движението на робота.
 * Това е основната функция за контрол на робота чрез клавиатурата.
 */
static void key_callback(GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mode)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
		case GLFW_KEY_ESCAPE:   // При натискане на ESC затваряме прозореца и приключваме изпълнението на програмата
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_X:    // При натискане на X извеждаме грешки от OpenGL в конзолата
            gl_print_error();   
            break;

		// Тук дефинираме управлението на робота (контролите, с които ще движим робота в различни посоки по екрана)
        case GLFW_KEY_W:    
            // Движение НАПРЕД: 
            // - Изчисляваме посоката спрямо текущия ъгъл на завъртане на робота
            // - sin/cos се използват за движение в правилната посока във световното пространство
            // - Умножаваме по скоростта на движение за контролиране на интензитета
            cg::robot.position.x -= sin(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            cg::robot.position.z -= cos(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            break;
        case GLFW_KEY_S:
            // Движение НАЗАД:
            // - Обратно на движение напред
            // - Добавяме към позицията вместо да изваждаме
            cg::robot.position.x += sin(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            cg::robot.position.z += cos(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            break;
        case GLFW_KEY_A:
            // ЗАВЪРТАНЕ НАЛЯВО:
            // - Увеличаваме ъгъла на завъртане по оста Y (ротация)
            // - Положителна стойност върти робота наляво
            cg::robot.rotation.y += g_rotation_speed;
            break;
        case GLFW_KEY_D:
            // ЗАВЪРТАНЕ НАДЯСНО:
            // - Намаляваме ъгъла на завъртане по оста Y (ротация)
            // - Отрицателна стойност върти робота надясно
            cg::robot.rotation.y -= g_rotation_speed;
            break;
        case GLFW_KEY_Q: 
            // СТРАФИРАНЕ НАЛЯВО (странично движение):
            // - Движение наляво без промяна на посоката на гледане
            // - Използваме косинус за хоризонтално движение
            // - Комбинираме със синус за движение по оста Z
            cg::robot.position.x -= cos(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            cg::robot.position.z += sin(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            break;
        case GLFW_KEY_E: 
            // СТРАФИРАНЕ НАДЯСНО (странично движение):
            // - Движение надясно без промяна на посоката на гледане
            // - Обратно на движение наляво
            cg::robot.position.x += cos(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            cg::robot.position.z -= sin(glm::radians(cg::robot.rotation.y)) * g_move_speed;
            break;
        case GLFW_KEY_SPACE: 
            // ДВИЖЕНИЕ НАГОРЕ:
            // - Просто увеличаваме Y координатата
            // - Движение по вертикалната ос
            cg::robot.position.y += g_move_speed;
            break;
        case GLFW_KEY_LEFT_SHIFT: 
            // ДВИЖЕНИЕ НАДОЛУ:
            // - Намаляваме Y координатата
            // - Движение по вертикалната ос надолу
            cg::robot.position.y -= g_move_speed;
            break;
        default:
            break;
        }
    }
}

/*
 * Callback функция за промяна на размера на прозореца.
 * Когато потребителят преоразмерява прозореца, актуализираме изгледа и проекцията.
 */
static void size_callback(GLFWwindow* window, int width, int height)
{
    if (width == 0 || height == 0)
        return;

    glViewport(0, 0, width, height);
    cg::perspective.aspect = static_cast<float>(width) / height;
    set_projection(g_program);
};

/*
 * Създаване на графичен прозорец.
 * Инициализира GLFW и създава прозорец с OpenGL контекст.
 */
GLFWwindow* init_window(void)
{
    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() == GLFW_FALSE)
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cg::version.gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cg::version.gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    glfwWindowHint(GLFW_SAMPLES, 4);

    /*
     * Създаване на графичния контекст и правене му текущ.
     */
    GLFWwindow* const window = glfwCreateWindow(cg::window.window_width,
        cg::window.window_height,
        cg::window.window_title,
        nullptr,
        nullptr);

    if (window == nullptr)
        return nullptr;

    glfwMakeContextCurrent(window);     // Правене на контекста текущ

    /*
     * Включване на VSync (вертикална синхронизация).
     */
    glfwSwapInterval(1);

    /*
     * Задаване на callback функции за събития.
     */
    glfwSetKeyCallback(window, key_callback);           // Callback за клавиши
    glfwSetWindowSizeCallback(window, size_callback);   // Callback за преоразмеряване

    /*
     * Зареждане на OpenGL функции с GLAD.
     * Програмата ще се срине ако се извикат OpenGL функции без това.
     */
    gladLoadGL();

    return window;
}

/*
 * Почистване на цветовия и дълбочинен буфер.
 * Подготвя екрана за нов кадър.
 */
static void clear(void)
{
    glClearColor(clear_color.x * clear_color.w,
        clear_color.y * clear_color.w,
        clear_color.z * clear_color.w,
        clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
 * Унищожаване на прозореца и почистване на ресурси.
 */
static void cleanup_window(GLFWwindow* window)
{
    glfwDestroyWindow(window);  // Унищожаване на прозореца
    glfwTerminate();    // Затваряне на GLFW
}

/*
 * Прочитане на шейдър от файл.
 * Връща съдържанието на файла като string.
 */
static std::optional<std::string> read_shader(const std::string& path)
{
    std::string result;

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (in.good() == false)
        return std::nullopt;

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();

    if (size == -1)
        return std::nullopt;

    result.resize(size);
    in.seekg(0, std::ios::beg);
    in.read(&result[0], size);

    return result;
}

/*
 * Компилиране на шейдър.
 * Копираме изходния код на шейдъра тук за всеки случай.
 * c_str ще сочи към грешни данни ако референцията излезе от обхват.
 */
static unsigned int compile_shader(const std::string shader_source,
    unsigned int type)
{
    unsigned int shader = glCreateShader(type);     // Създаване на шейдър обект

    const char* c_str = shader_source.c_str();
    glShaderSource(shader, 1, &c_str, nullptr);     // Задаване на изходния код

    glCompileShader(shader);    // Компилиране на шейдъра

    /*
     * Проверка за грешки при компилация.
     */
    int is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == 0)
    {
        std::string log;
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);     // Взимане на дължината на съобщението за грешка
        log.resize(length);
        glGetShaderInfoLog(shader, length, nullptr, &log[0]);   // Взимане на съобщението за грешка

        std::string type_s = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
        std::cerr << "Failed to compile " << type_s << " shader." << std::endl;
        std::cerr << log << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

/*
 * Компилиране и свързване на шейдърна програма.
 * Свързва vertex и fragment шейдъри в една програма.
 */
static unsigned int create_shader(const std::string& vertex_source,
    const std::string& fragment_source)
{
	unsigned int program = glCreateProgram();   // Създаване на шейдърна програма
    unsigned int vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (vertex_shader == 0)
        return 0;
    unsigned int fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (fragment_shader == 0)
        return 0;

    glAttachShader(program, vertex_shader);     // Прикачане на vertex шейдъра
    glAttachShader(program, fragment_shader);   // Прикачане на fragment шейдъра
    glLinkProgram(program);     // Свързване на програмата
    glValidateProgram(program);     // Валидиране на програмата

    glDeleteShader(vertex_shader);      // Изтриване на шейдърите след свързване
    glDeleteShader(fragment_shader);

    glDetachShader(program, vertex_shader);     // Откачане на шейдърите
    glDetachShader(program, fragment_shader);

    return program;
}

/*
 * Взимане на локация на uniform променлива в шейдъра.
 * Използва кеш за по-бърз достъп.
 */
int get_uniform_location(unsigned int program, const std::string& location)
{
    if (g_uniform_locations.find(location) != g_uniform_locations.end())
        return g_uniform_locations[location];   // Връщаме от кеша ако вече има

    int uniform = glGetUniformLocation(program, location.c_str());      // Взимане на локация от OpenGL
    if (uniform == -1)
        std::cout << "Warning: Uniform " << location <<
        " does not exist. This uniform will not be set." << std::endl;

    g_uniform_locations[location] = uniform;       // Запазване в кеша
    return uniform;
}

/*
 * Задаване на vec3 uniform променлива в шейдъра.
 */
static void set_vec3(unsigned int program,
    const glm::vec3& vector,
    const std::string& location)
{
    int uniform = get_uniform_location(program, location);
    if (uniform == -1)
        return;
    glUniform3fv(uniform, 1, glm::value_ptr(vector));
}

/*
 * Задаване на matrix4 uniform променлива в шейдъра.
 */
static void set_matrix(unsigned int program,
    const glm::mat4& matrix,
    const std::string& location)
{
    int uniform = get_uniform_location(program, location);
    if (uniform == -1)
        return;
    glUniformMatrix4fv(uniform, 1, false, glm::value_ptr(matrix));
}

/*
 * Задаване на моделна матрица в шейдъра.
 * Моделната матрица описва трансформациите на обекта (позиция, ротация, мащаб).
 */
static void set_model(unsigned int program)
{
    set_matrix(program, g_model, "u_model");
}

/*
 * Задаване на view матрица в шейдъра.
 * View матрицата описва позицията и ориентацията на камерата.
 */
static void set_view(unsigned int program)
{
    glm::mat4 view = glm::lookAt(cg::camera.eye,    // Позиция на камерата
        cg::camera.center,      // Точка, към която гледа камерата
        cg::camera.up);     // Вектор на "нагоре" за камерата

    set_matrix(program, view, "u_view");
    set_vec3(program, cg::camera.eye, "u_view_pos");    // Подаване и на позицията на камерата
}

/*
 * Задаване на проекционна матрица в шейдъра.
 * Проекционната матрица преобразува 3D координати в 2D координати на екрана.
 */
static void set_projection(unsigned int program)
{
    glm::mat4 projection = glm::perspective(cg::perspective.fov,    // Поле на зрение
        cg::perspective.aspect,     // Съотношение на страните
        cg::perspective.z_near,     // Близка равнина за отсичане
        cg::perspective.z_far);     // Далечна равнина за отсичане

    set_matrix(program, projection, "u_projection");
}

/*
 * Задаване на позиция на светлината в шейдъра.
 */
static void set_light_pos(unsigned int program)
{
    set_vec3(program, g_light_pos, "u_light_pos");
}

/*
 * Задаване на цвят на светлината в шейдъра.
 */
static void set_light_color(unsigned int program)
{
    set_vec3(program, g_light_color, "u_light_color");
}

/*
 * Vertex Buffer Object (VBO).
 * Качва данните за рисуване в паметта на GPU.
 * Съдържа върховите данни на куб (позиция, нормала, текстурни координати).
 */
static unsigned int init_vbo(void)
{
    /*
     * Дефиниране на куб.
     * Всеки ред съдържа: X,Y,Z позиция, X,Y,Z нормала, U,V текстурни координати
     */
    std::array<float, 288> vertices =
    {
        /* Position            Normal                Texture Coords */
        /* Front face - Предна страна */
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,

         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,

        /* Back face - Задна страна */
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,

         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,

        /* Left face - Лява страна */
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

        /* Right face - Дясна страна */
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

         /* Top face - Горна страна */
         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
          0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,

          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,

         /* Bottom face - Долна страна */
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
          0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,

          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
         -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f
    };

    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);      // Генериране на VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);     // Свързване на VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);      // Качване на данните в GPU паметта

    return vbo;
}

/*
 * Vertex Array Object (VAO).
 * Специфицира формата на данните за рисуване.
 * Описва как да се интерпретират данните от VBO.
 */
static unsigned int init_vao(void)
{
    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);     // Генериране на VAO
    glBindVertexArray(vao);     // Свързване на VAO

    /*
     * Атрибут за позиция (локация 0).
     * 3 float числа, стъпка 8*sizeof(float), отместване 0.
     */
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /*
     * Атрибут за нормала (локация 1).
     * 3 float числа, стъпка 8*sizeof(float), отместване 3*sizeof(float).
     */
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /*
     * Атрибут за текстурни координати (локация 2).
     * 2 float числа, стъпка 8*sizeof(float), отместване 6*sizeof(float).
     */
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return vao;
}

/*
 * Зареждане и свързване на текстура от изображение.
 * Използва библиотеката stb_image за зареждане на изображения.
 */
static unsigned int init_texture(const std::string& path)
{
    int texture_width = 0;
    int texture_height = 0;
    int texture_bpp = 0;

    stbi_set_flip_vertically_on_load(1);        // Обръщане на изображението вертикално (OpenGL координати)

    unsigned char* texture_data = stbi_load(path.c_str(),
        &texture_width,
        &texture_height,
        &texture_bpp,
        4);     // Зареждане като RGBA (4 канала)

    if (texture_data == nullptr)
    {
        std::cerr << "Failed to load texture." << std::endl;
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);     // Генериране на текстура
    glBindTexture(GL_TEXTURE_2D, texture);      // Свързване на текстурата

    /*
     * Настройки за увиване и филтриране на текстурата.
     * Задължителни настройки.
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    // Увиване по X
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    // Увиване по Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);       // Филтър при намаляване
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);       // Филтър при увеличаване

    glTexImage2D(GL_TEXTURE_2D,     // Качване на данните за текстурата в GPU
        0,
        GL_RGBA8,
        texture_width,
        texture_height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture_data);

    glGenerateMipmap(GL_TEXTURE_2D);        // Генериране на мипмапове
    glActiveTexture(GL_TEXTURE0);           // Активиране на текстурен unit 0
    glBindTexture(GL_TEXTURE_2D, texture);  // Свързване на текстурата

    stbi_image_free(texture_data);      // Освобождаване на паметта за изображението
    return texture;
}

/*
 * Настройка на шейдърна програма.
 * Зарежда, компилира и свързва vertex и fragment шейдъри.
 */
static unsigned int init_program(const std::string& vertex_path,
    const std::string& fragment_path)
{
    const auto vertex_source = read_shader(vertex_path);        // Прочитане на vertex шейдъ
    const auto fragment_source = read_shader(fragment_path);    // Прочитане на fragment шейдър
    if (vertex_source.has_value() == false ||
        fragment_source.has_value() == false)
    {
        std::cerr << "Failed to read shaders." << std::endl;
        return 0;
    }

    unsigned int program = create_shader(vertex_source.value(),
        fragment_source.value());       // Създаване на шейдърна програма

    return program;
}

/*
 * Инициализация на сцената.
 * Настройва OpenGL състояния, зарежда данни и шейдъри.
 */
static void init(void)
{
    /*
     * Включване на Z-буфер (дълбочина) и мултисемплиране.
     */
    glEnable(GL_DEPTH_TEST);    // Включване на тест за дълбочина
    glEnable(GL_MULTISAMPLE);   // Включване на анти-алиасинг

    /*
     * Настройки за прозрачност (blending).
     */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Функция за смесване
    glEnable(GL_BLEND);     // Включване на blending

    unsigned int vbo = init_vbo();  // Инициализация на VBO
    unsigned int vao = init_vao();  // Инициализация на VAO
    unsigned int texture = init_texture("resources/textures/tu_white.png");     // Зареждане на текстура

    std::cout << "Data init check:" << std::endl;
    if (gl_print_error() != 0)   // Проверка за грешки
        return;

    unsigned int program = init_program("resources/shaders/tex_v.glsl",     // Зареждане на шейдъри
        "resources/shaders/tex_f.glsl");

    if (program == 0)
    {
        std::cerr << "Failed to compile shaders." << std::endl;
        return;
    }

    glUseProgram(program);
    g_program = program;

    /*
     * Set PVM matrix.
     */
    set_model(program);
    set_view(program);
    set_projection(program);

    /*
     * Set light parameters.
     */
    set_light_pos(program);
    set_light_color(program);
}

/*
 * Draw a cuboid (scaled cube)
 */
static void draw_cuboid(const glm::vec3& size)
{
    glm::mat4 original_model = g_model;
    g_model = glm::scale(g_model, size);
    set_model(g_program);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    g_model = original_model;
}

/*
 * Draw the complete robot
 */
static void draw_robot()
{
    glm::mat4 original_model = g_model;

    // Apply robot transformation
    g_model = glm::translate(g_model, cg::robot.position);
    g_model = glm::rotate(g_model, glm::radians(cg::robot.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // Hips
    g_model = glm::translate(g_model, glm::vec3(0.0f, cg::robot.hip_size.y / 2, 0.0f));
    draw_cuboid(cg::robot.hip_size);

    // Body (on top of hips)
    glm::mat4 hips_model = g_model;
    g_model = glm::translate(g_model, glm::vec3(0.0f, cg::robot.hip_size.y / 2 + cg::robot.body_size.y / 2, 0.0f));
    draw_cuboid(cg::robot.body_size);

    // Shoulders (on top of body) - FIXED: Make shoulders wider to match body
    glm::mat4 body_model = g_model;
    g_model = glm::translate(g_model, glm::vec3(0.0f, cg::robot.body_size.y / 2 + cg::robot.shoulder_size.y / 2, 0.0f));
    // Make shoulders wider to prevent thin neck look
    glm::vec3 shoulder_draw_size = glm::vec3(
        cg::robot.body_size.x * 1.1f,  // Wider than body
        cg::robot.shoulder_size.y,
        cg::robot.body_size.z * 0.8f   // Slightly narrower in depth
    );
    draw_cuboid(shoulder_draw_size);

    // Head (on top of shoulders)
    glm::mat4 shoulders_model = g_model;
    g_model = glm::translate(g_model, glm::vec3(0.0f, cg::robot.shoulder_size.y / 2 + cg::robot.head_size.y / 2, 0.0f));
    draw_cuboid(cg::robot.head_size);

    // Eyes
    glm::mat4 head_model = g_model;
    g_model = glm::translate(g_model, glm::vec3(cg::robot.head_size.x / 4, cg::robot.head_size.y / 4, cg::robot.head_size.z / 2 + cg::robot.eye_size.z / 2));
    draw_cuboid(cg::robot.eye_size);

    g_model = head_model;
    g_model = glm::translate(g_model, glm::vec3(-cg::robot.head_size.x / 4, cg::robot.head_size.y / 4, cg::robot.head_size.z / 2 + cg::robot.eye_size.z / 2));
    draw_cuboid(cg::robot.eye_size);

    // Antenna
    g_model = head_model;
    g_model = glm::translate(g_model, glm::vec3(0.0f, cg::robot.head_size.y / 2 + cg::robot.antenna_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.antenna_wiggle), glm::vec3(0.0f, 0.0f, 1.0f));
    draw_cuboid(cg::robot.antenna_size);

    // Left Arm - FIXED: Position arms further out to avoid body overlap
    g_model = shoulders_model;
    g_model = glm::translate(g_model, glm::vec3(-shoulder_draw_size.x / 2 - cg::robot.arm_size.x / 2,
        0.0f, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.arm_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    // Move down to center the arm at the attachment point
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.arm_size.y / 2, 0.0f));
    draw_cuboid(cg::robot.arm_size);

    // Left Forearm
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.arm_size.y / 2 - cg::robot.forearm_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.forearm_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.forearm_size);

    // Right Arm - FIXED: Position arms further out to avoid body overlap
    g_model = shoulders_model;
    g_model = glm::translate(g_model, glm::vec3(shoulder_draw_size.x / 2 + cg::robot.arm_size.x / 2,
        0.0f, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(-cg::robot.arm_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    // Move down to center the arm at the attachment point
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.arm_size.y / 2, 0.0f));
    draw_cuboid(cg::robot.arm_size);

    // Right Forearm
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.arm_size.y / 2 - cg::robot.forearm_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(-cg::robot.forearm_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.forearm_size);

    // Left Leg
    g_model = hips_model;
    g_model = glm::translate(g_model, glm::vec3(-cg::robot.hip_size.x / 4,
        -cg::robot.hip_size.y / 2 - cg::robot.leg_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(-cg::robot.leg_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.leg_size);

    // Left Shin
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.leg_size.y / 2 - cg::robot.shin_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(-cg::robot.shin_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.shin_size);

    // Right Leg
    g_model = hips_model;
    g_model = glm::translate(g_model, glm::vec3(cg::robot.hip_size.x / 4,
        -cg::robot.hip_size.y / 2 - cg::robot.leg_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.leg_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.leg_size);

    // Right Shin
    g_model = glm::translate(g_model, glm::vec3(0.0f, -cg::robot.leg_size.y / 2 - cg::robot.shin_size.y / 2, 0.0f));
    g_model = glm::rotate(g_model, glm::radians(cg::robot.shin_swing), glm::vec3(1.0f, 0.0f, 0.0f));
    draw_cuboid(cg::robot.shin_size);

    g_model = original_model;
}

/*
 * Draw starfield background
 */
static void draw_starfield_background()
{
    // Save current state
    glm::mat4 original_model = g_model;

    // Create a large sphere/cube for the starfield
    g_model = glm::mat4(1.0f);
    g_model = glm::translate(g_model, cg::camera.eye); // Follow camera
    g_model = glm::scale(g_model, glm::vec3(100.0f));  // Large scale

    set_model(g_program);

    // Draw the background
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glEnable(GL_DEPTH_TEST);

    // Restore
    g_model = original_model;
    set_model(g_program);
}

/*
 * Draw function.
 */
static void render(void)
{
    // Update walking animation
    static float time = 0.0f;
    time += 0.05f;  // Reduced from 0.1f to 0.05f to slow down overall animation

    // Main arm and leg swings - slower
    cg::robot.arm_swing = sin(time * cg::robot.walk_speed) * 30.0f;
    cg::robot.leg_swing = sin(time * cg::robot.walk_speed) * 25.0f;

    // Forearms and shins - even slower than main limbs
    cg::robot.forearm_swing = sin(time * cg::robot.walk_speed * 0.8f) * 15.0f;  // Reduced multiplier
    cg::robot.shin_swing = sin(time * cg::robot.walk_speed * 0.7f) * 20.0f;     // Reduced multiplier

    // Head and antenna - keep their original speed for variety
    cg::robot.head_bob = sin(time * cg::robot.walk_speed * 2.0f) * 3.0f;
    cg::robot.antenna_wiggle = sin(time * cg::robot.walk_speed * 3.0f) * 10.0f;

    draw_robot();
}

/*
 * Create window and begin drawing.
 */
static void run(void)
{
    GLFWwindow* window = init_window();
    if (window == nullptr)
        std::exit(1);

    cg::init_ImGui(window);
    init();

    /*
     * Main loop.
     * Runs every frame.
     */
    while (glfwWindowShouldClose(window) == 0)
    {
        glfwPollEvents();

        cg::render_ImGui();

        clear();

        render();

        cg::display_ImGui();

        glfwSwapBuffers(window);
    }

    /*
     * Cleanup.
     */
    cg::cleanup_ImGui();
    cleanup_window(window);
}

int main(void)
{
    /*
     * Keep main function brief.
     */
    run();
}