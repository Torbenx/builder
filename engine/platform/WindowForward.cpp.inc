namespace plt {
    
Window Window::construct() {
    return Window(new WINDOW_IMPL);
}

void Window::setTitle(std::string_view title) {
    const_cast<WINDOW_IMPL*>(static_cast<const WINDOW_IMPL*>(windowBackend()))->setTitle(title);
}
std::string_view Window::title() const {
    return static_cast<const WINDOW_IMPL*>(windowBackend())->title();
}

void Window::setDimensions(ui::IntVec dim) {
    const_cast<WINDOW_IMPL*>(static_cast<const WINDOW_IMPL*>(windowBackend()))->setDimensions(dim);
}

void Window::initialize() {
    const_cast<WINDOW_IMPL*>(static_cast<const WINDOW_IMPL*>(windowBackend()))->initialize();
}
void Window::destroy() {
    auto* window = const_cast<WINDOW_IMPL*>(static_cast<const WINDOW_IMPL*>(windowBackend()));
    window->destroy();
    // delete window;
}

}