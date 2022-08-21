namespace plt {
std::unique_ptr<Window> Window::construct() {
    return std::make_unique<WINDOW_IMPL>();
}

void Window::setTitle(std::string_view title) {
    return static_cast<WINDOW_IMPL*>(this)->setTitle(title);
}
std::string_view Window::title() const {
    return static_cast<const WINDOW_IMPL*>(this)->title();
}

void Window::setStateCallback(WindowStateCallback* callback) {
    return static_cast<WINDOW_IMPL*>(this)->setStateCallback(callback);
}
WindowStateCallback* Window::stateCallback() const {
    return static_cast<const WINDOW_IMPL*>(this)->stateCallback();
}

void Window::setDimensions(int_t width, int_t height) {
    return static_cast<WINDOW_IMPL*>(this)->setDimensions(width, height);
}
int_t Window::width() const {
    return static_cast<const WINDOW_IMPL*>(this)->width();
}
int_t Window::height() const {
    return static_cast<const WINDOW_IMPL*>(this)->height();
}

void Window::initialize() {
    return static_cast<WINDOW_IMPL*>(this)->initialize();
}
void Window::destroy() {
    return static_cast<WINDOW_IMPL*>(this)->destroy();
}
WindowState Window::state() const {
    return static_cast<const WINDOW_IMPL*>(this)->state();
}

Window::~Window() {
    static_cast<WINDOW_IMPL*>(this)->destructor();
}
}