#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ui.h"
#include "structs.h"

// Добавяне на extern декларации за достъп до глобалните променливи от главния файл
// Тези променливи се дефинират в main файла и тук само декларираме, че ще ги използваме
extern float g_move_speed;        // Скорост на движение на робота
extern float g_rotation_speed;    // Скорост на завъртане на робота

namespace cg
{
    /*
     * Инициализация на ImGui системата.
     * Създава ImGui контекст и го свързва с GLFW и OpenGL.
     */
    void init_ImGui(GLFWwindow* window)
    {
        IMGUI_CHECKVERSION();           // Проверка на версията на ImGui
        ImGui::CreateContext();         // Създаване на ImGui контекст
        ImGuiIO& io = ImGui::GetIO();   // Вземане на достъп до Input/Output настройките
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Активиране на клавиатурна навигация

        ImGui::StyleColorsDark();   // Задаване на тъмна тема за интерфейса

        // Инициализация на бекенда за GLFW
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        // Инициализация на бекенда за OpenGL с конкретна GLSL версия
        ImGui_ImplOpenGL3_Init(cg::version.glsl_version);
    }

    /*
     * Рендиране на ImGui интерфейса.
     * Създава и обновява всички ImGui прозорци и елементи.
     */
    void render_ImGui(void)
    {
        bool show_demo_window = false; // Флаг за показване на демо прозореца на ImGui

        // Подготвяне на нов кадър за ImGui
        ImGui_ImplOpenGL3_NewFrame();   // Нов кадър за OpenGL
        ImGui_ImplGlfw_NewFrame();      // Нов кадър за GLFW
        ImGui::NewFrame();              // Нов кадър за ImGui

        // Показване на демо прозореца на ImGui (ако е активиран)
        if (show_demo_window == true)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

		// Прозорец за контрол на робота

        // Създаване на прозореца "Robot Controls"
        ImGui::Begin("Robot Controls");

        // Показване на текущата позиция на робота
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
            cg::robot.position.x, cg::robot.position.y, cg::robot.position.z);

        // Показване на текущите ъгли на завъртане на робота
        ImGui::Text("Rotation: (%.2f, %.2f, %.2f)",
            cg::robot.rotation.x, cg::robot.rotation.y, cg::robot.rotation.z);

        ImGui::Separator(); // Хоризонтална разделителна линия

        // Секция с инструкции за управление
        ImGui::Text("Controls:");
        ImGui::Text("W/S - Move Forward/Back");        // Движение напред/назад
        ImGui::Text("A/D - Rotate Left/Right");        // Завъртане наляво/надясно
        ImGui::Text("Q/E - Strafe Left/Right");        // Странично движение
        ImGui::Text("Space/Shift - Move Up/Down");     // Движение нагоре/надолу

        ImGui::Separator();     // Хоризонтална разделителна линия

        // Секция за настройка на скоростите
        ImGui::SliderFloat("Move Speed", &g_move_speed, 0.01f, 1.0f);           // Плъзгач за скорост на движение
        ImGui::SliderFloat("Rotation Speed", &g_rotation_speed, 0.5f, 10.0f);   // Плъзгач за скорост на завъртане
        ImGui::SliderFloat("Walk Speed", &cg::robot.walk_speed, 0.5f, 5.0f);    // Плъзгач за скорост на анимацията

        // Секция за контрол на размерите на частите на робота
        ImGui::Separator();
        ImGui::Text("Robot Parts:");

        // Плъзгачи за основните части на робота
        ImGui::SliderFloat3("Body Size", &cg::robot.body_size[0], 0.1f, 2.0f);   // Размер на тялото (X,Y,Z)
        ImGui::SliderFloat3("Head Size", &cg::robot.head_size[0], 0.1f, 1.0f);   // Размер на главата
        ImGui::SliderFloat3("Arm Size", &cg::robot.arm_size[0], 0.1f, 1.0f);     // Размер на ръцете
        ImGui::SliderFloat3("Leg Size", &cg::robot.leg_size[0], 0.1f, 1.0f);     // Размер на краката

        // Плъзгачи за сложните части на робота (добавени по-късно)
        ImGui::SliderFloat3("Shoulder Size", &cg::robot.shoulder_size[0], 0.1f, 1.0f);   // Размер на раменете
        ImGui::SliderFloat3("Hip Size", &cg::robot.hip_size[0], 0.1f, 1.0f);             // Размер на ханша
        ImGui::SliderFloat3("Forearm Size", &cg::robot.forearm_size[0], 0.1f, 1.0f);     // Размер на предмишниците
        ImGui::SliderFloat3("Shin Size", &cg::robot.shin_size[0], 0.1f, 1.0f);           // Размер на пищялите
        ImGui::SliderFloat3("Eye Size", &cg::robot.eye_size[0], 0.01f, 0.2f);            // Размер на очите (по-малък диапазон)
        ImGui::SliderFloat3("Antenna Size", &cg::robot.antenna_size[0], 0.01f, 0.5f);    // Размер на антената

        // Секция за контрол на анимациите
        ImGui::Separator();
        ImGui::Text("Animation Controls:");

        // Плъзгачи за контрол на анимациите на различните части
        ImGui::SliderFloat("Arm Swing", &cg::robot.arm_swing, 0.0f, 45.0f);           // Анимация на движение на ръцете
        ImGui::SliderFloat("Leg Swing", &cg::robot.leg_swing, 0.0f, 45.0f);           // Анимация на движение на краката
        ImGui::SliderFloat("Forearm Swing", &cg::robot.forearm_swing, 0.0f, 30.0f);   // Анимация на движение на предмишниците
        ImGui::SliderFloat("Shin Swing", &cg::robot.shin_swing, 0.0f, 30.0f);         // Анимация на движение на пищялите
        ImGui::SliderFloat("Head Bob", &cg::robot.head_bob, 0.0f, 10.0f);             // Анимация на движение на главата
        ImGui::SliderFloat("Antenna Wiggle", &cg::robot.antenna_wiggle, 0.0f, 20.0f); // Анимация на движение на антената

        // Бутон за нулиране на робота към начални стойности
        if (ImGui::Button("Reset Robot")) {
            // Нулиране на позицията и ротацията
            cg::robot.position = glm::vec3(0.0f, 0.0f, 0.0f);   // Център на сцената, леко надолу
            cg::robot.rotation = glm::vec3(0.0f);               // Без завъртане

            // Възстановяване на размерите на основните части
            cg::robot.body_size = glm::vec3(0.8f, 1.2f, 0.4f);  // Стандартен размер на тялото
            cg::robot.head_size = glm::vec3(0.5f, 0.5f, 0.5f);  // Стандартен размер на главата
            cg::robot.arm_size = glm::vec3(0.2f, 1.0f, 0.2f);   // Стандартен размер на ръцете
            cg::robot.leg_size = glm::vec3(0.2f, 1.0f, 0.2f);   // Стандартен размер на краката

            // Възстановяване на размерите на сложните части
            cg::robot.shoulder_size = glm::vec3(0.3f, 0.3f, 0.3f);   // Стандартен размер на раменете
            cg::robot.hip_size = glm::vec3(0.4f, 0.3f, 0.4f);        // Стандартен размер на ханша
            cg::robot.forearm_size = glm::vec3(0.15f, 0.6f, 0.15f);  // Стандартен размер на предмишниците
            cg::robot.shin_size = glm::vec3(0.15f, 0.6f, 0.15f);     // Стандартен размер на пищялите
            cg::robot.eye_size = glm::vec3(0.08f, 0.08f, 0.08f);     // Стандартен размер на очите
            cg::robot.antenna_size = glm::vec3(0.05f, 0.3f, 0.05f);  // Стандартен размер на антената

            // Нулиране на всички анимационни стойности
            cg::robot.arm_swing = 0.0f;        // Без движение на ръцете
            cg::robot.leg_swing = 0.0f;        // Без движение на краката
            cg::robot.forearm_swing = 0.0f;    // Без движение на предмишниците
            cg::robot.shin_swing = 0.0f;       // Без движение на пищялите
            cg::robot.head_bob = 0.0f;         // Без движение на главата
            cg::robot.antenna_wiggle = 0.0f;   // Без движение на антената
            cg::robot.walk_speed = 2.0f;       // Стандартна скорост на анимация
        }

        ImGui::End(); // Край на прозореца "Robot Controls"

        ImGui::Render(); // Рендиране на всички ImGui елементи
    }

    /*
     * Показване на рендирания ImGui интерфейс на екрана.
     * Това трябва да се извика след като ImGui::Render() е бил извикан.
     */
    void display_ImGui(void)
    {
        // Рендиране на ImGui данните чрез OpenGL бекенда
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    /*
     * Почистване на ImGui ресурси.
     * Трябва да се извика при затваряне на приложението.
     */
    void cleanup_ImGui(void)
    {
        // Затваряне на OpenGL бекенда
        ImGui_ImplOpenGL3_Shutdown();
        // Затваряне на GLFW бекенда
        ImGui_ImplGlfw_Shutdown();
        // Унищожаване на ImGui контекста
        ImGui::DestroyContext();
    }

} // namespace cg