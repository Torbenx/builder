#include "BigStyleTable.hpp"
#include "StyleKey.hpp"
#include "common.hpp"

namespace ui {

Ref<BigStyleTable> Ref<BigStyleTable>::allocate() {
    int_t keyCount = __end_ui_style_keys - __begin_ui_style_keys;
    log<Tag::Ui>("found {} style keys", keyCount);
    void** data = new void*[keyCount + 1]();
    data[0] = nullptr;
    for (int_t i = 0; i < keyCount; i++) {
        const StyleKeyRecord& record = __begin_ui_style_keys[i];
        data[i + 1] = new (std::align_val_t(record.valueAlignment)) std::byte[record.valueSize]();
    }
    return { data };
}

BigStyleTable Ref<BigStyleTable>::finalize() const {
    int_t keyCount = __end_ui_style_keys - __begin_ui_style_keys;
    void** output = new void*[keyCount + 1]();
    output[0] = nullptr;
    Resolver resolver { .table = *this };
    for (int_t i = 0; i < keyCount; i++) {
        const StyleKeyRecord& record = __begin_ui_style_keys[i];
        output[i + 1] = new (std::align_val_t(record.valueAlignment)) std::byte[record.valueSize]();
        record.resolveFunc(output[i + 1], data[i + 1], resolver);
    }
    return { output };
}

}