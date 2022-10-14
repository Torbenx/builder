namespace plt {

void Window::setTitle(std::string_view title) {
    TO_WINDOW_IMPL(*this)->setTitle(title);
}
std::string_view Window::title() const {
    return TO_WINDOW_IMPL(*this)->title();
}

void Window::setDimensions(ui::IntVec dim) {
    TO_WINDOW_IMPL(*this)->setDimensions(dim);
}

void Window::initialize() {
    TO_WINDOW_IMPL(*this)->initialize();
}
void Window::destroy() {
    TO_WINDOW_IMPL(*this)->destroy();
}

}