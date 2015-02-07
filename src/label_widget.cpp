#include "label_widget.hpp"
#include "gui.hpp"

LabelWidget::LabelWidget(Gui *gui, int gui_index) :
        _gui_index(gui_index),
        _label(gui),
        _is_visible(true),
        _padding_left(4),
        _padding_right(4),
        _padding_top(4),
        _padding_bottom(4),
        _background_color(0.788f, 0.812f, 0.886f, 1.0f),
        _has_background(true),
        _gui(gui),
        _cursor_start(-1),
        _cursor_end(-1),
        _select_down(false)
{
    update_model();
    _label.set_color(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void LabelWidget::draw(const glm::mat4 &projection) {
    glm::mat4 bg_mvp = projection * _bg_model;
    _gui->fill_rect(_background_color, bg_mvp);

    glm::mat4 label_mvp = projection * _label_model;
    _label.draw(label_mvp);
}

void LabelWidget::update_model() {
    float label_left = _left + _padding_left;
    float label_top = _top + _padding_top;
    _label_model = glm::translate(glm::mat4(1.0f), glm::vec3(label_left, label_top, 0.0f));


    _bg_model = glm::scale(
                        glm::translate(
                            glm::mat4(1.0f),
                            glm::vec3(_left, _top, 0.0f)),
                        glm::vec3(width(), height(), 0.0f));
}

void LabelWidget::on_mouse_over(const MouseEvent &event) {
    SDL_SetCursor(_gui->_cursor_ibeam);
}

void LabelWidget::on_mouse_out(const MouseEvent &event) {
    SDL_SetCursor(_gui->_cursor_default);
}

void LabelWidget::on_mouse_move(const MouseEvent &event) {
    switch (event.action) {
        case MouseActionDown:
            if (event.button == MouseButtonLeft) {
                int index = cursor_at_pos(event.x, event.y);
                _cursor_start = index;
                _cursor_end = index;
                _select_down = true;
                break;
            }
        case MouseActionUp:
            if (event.button == MouseButtonLeft && _select_down) {
                _select_down = false;
            }
            break;
        case MouseActionMove:
            if (event.buttons.left && _select_down) {
                _cursor_end = cursor_at_pos(event.x, event.y);

                int start, end;
                if (_cursor_start <= _cursor_end) {
                    start = _cursor_start;
                    end = _cursor_end + 1;
                } else {
                    start = _cursor_end;
                    end = _cursor_start;
                }
                int len = _label.text().length();
                end = (end > len) ? len : end;
                String selected = _label.text().substring(start, end);
                fprintf(stderr, "%s\n", selected.encode().raw());
            }
            break;
    }
}

int LabelWidget::cursor_at_pos(int x, int y) const {
    int inner_x = x - _padding_left;
    int inner_y = y - _padding_top;
    return _label.cursor_at_pos(inner_x, inner_y);
}