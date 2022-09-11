#include "BigStyle.hpp"
#include "StyleKey.hpp"

namespace ui {

Ref<BigStyle> Ref<BigStyle>::allocate() {
    int_t keyCount = __end_ui_style_keys - __begin_ui_style_keys;
    log<Tag::Ui>("found {} style keys", keyCount);
    void** data = new void*[keyCount]();
    for (int_t i = 0; i < keyCount; i++) {
        const StyleKeyRecord& record = __begin_ui_style_keys[i];
        data[i] = new std::byte[record.styleRefBytes]();
    }
    return { data };
}

}