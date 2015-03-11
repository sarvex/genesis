#ifndef GUI_WINDOW_HPP
#define GUI_WINDOW_HPP

#include "list.hpp"
#include "glm.hpp"
#include "key_event.hpp"
#include "mouse_event.hpp"
#include "string.hpp"
#include "glfw.hpp"

class Gui;
class Widget;
class TextWidget;
class FindFileWidget;
class AudioEditWidget;
class SpritesheetImage;

class GuiWindow {
public:
    GuiWindow(Gui *gui, bool is_normal_window);
    ~GuiWindow();

    void bind();
    void draw();

    TextWidget *create_text_widget();
    FindFileWidget *create_find_file_widget();
    AudioEditWidget *create_audio_edit_widget();

    void destroy_widget(Widget *widget);
    void set_focus_widget(Widget *widget);

    // return true if you ate the event
    void set_on_key_event(bool (*fn)(GuiWindow *, const KeyEvent *event)) {
        _on_key_event = fn;
    }
    // return true if you ate the event
    void set_on_text_event(bool (*fn)(GuiWindow *, const TextInputEvent *event)) {
        _on_text_event = fn;
    }

    void set_on_close_event(void (*fn)(GuiWindow *)) {
        _on_close_event = fn;
    }

    void set_cursor_beam();
    void set_cursor_default();

    bool try_mouse_move_event_on_widget(Widget *widget, const MouseEvent *event);

    void fill_rect(const glm::vec4 &color, int x, int y, int w, int h);
    void draw_image(const SpritesheetImage *img, int x, int y, int w, int h);

    void set_clipboard_string(const String &str);
    String get_clipboard_string() const;
    bool clipboard_has_string() const;


    void *_userdata;
    // index into Gui's list of windows
    int _gui_index;

    Gui *_gui;
    GLFWwindow *_window;

    // pixels
    int _width;
    int _height;

    // screen coordinates
    int _client_width;
    int _client_height;

private:

    glm::mat4 _projection;
    List<Widget *> _widget_list;
    Widget *_mouse_over_widget;
    Widget *_focus_widget;

    bool (*_on_key_event)(GuiWindow *, const KeyEvent *event);
    bool (*_on_text_event)(GuiWindow *, const TextInputEvent *event);
    void (*_on_close_event)(GuiWindow *);

    bool _is_iconified;
    bool _is_visible;

    double _last_click_time;
    MouseButton _last_click_button;
    double _double_click_timeout;

    void init_widget(Widget *widget);
    int get_modifiers();
    void on_mouse_move(const MouseEvent *event);

    void window_iconify_callback(int iconified);
    void framebuffer_size_callback(int width, int height);
    void window_size_callback(int width, int height);
    void key_callback(int key, int scancode, int action, int mods);
    void charmods_callback(unsigned int codepoint, int mods);
    void window_close_callback();
    void cursor_pos_callback(double xpos, double ypos);
    void mouse_button_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);

    static void static_window_iconify_callback(GLFWwindow* window, int iconified) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->window_iconify_callback(iconified);
    }
    static void static_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->framebuffer_size_callback(width, height);
    }
    static void static_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->key_callback(key, scancode, action, mods);
    }
    static void static_charmods_callback(GLFWwindow* window, unsigned int codepoint, int mods) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->charmods_callback(codepoint, mods);
    }
    static void static_window_close_callback(GLFWwindow* window) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->window_close_callback();
    }
    static void static_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->cursor_pos_callback(xpos, ypos);
    }
    static void static_window_size_callback(GLFWwindow* window, int width, int height) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->window_size_callback(width, height);
    }
    static void static_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->mouse_button_callback(button, action, mods);
    }
    static void static_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        return static_cast<GuiWindow*>(glfwGetWindowUserPointer(window))->scroll_callback(xoffset, yoffset);
    }
};

#endif