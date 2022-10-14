#include "BigStyleTable.hpp"
#include "StyleKey.hpp"
#include "StyleConsumer.hpp"

namespace ui {

Ref<BigStyleTable> Ref<BigStyleTable>::allocate() {
    auto section = records::ui_style_keys();
    log<Tag::Ui>("found {} style keys", section.size());
    void** data = new void*[section.size() + 1]();
    data[0] = nullptr;
    for (int_t i = 0; i < section.size(); i++) {
        const StyleKeyRecord& record = section[i];
        VERIFY(Ui, record.valueAlignment <= (int_t)__STDCPP_DEFAULT_NEW_ALIGNMENT__);
        data[i + 1] = new std::byte[record.valueSize]();
    }
    return { data };
}

BigStyleTable Ref<BigStyleTable>::finalize() const {
    auto section = records::ui_style_keys();
    void** output = new void*[section.size() + 1]();
    output[0] = nullptr;
    Resolver resolver { .table = *this };
    for (int_t i = 0; i < section.size(); i++) {
        const StyleKeyRecord& record = section[i];
        VERIFY(Ui, record.valueAlignment <= (int_t)__STDCPP_DEFAULT_NEW_ALIGNMENT__);
        output[i + 1] = new std::byte[record.valueSize]();
        record.resolveFunc(output[i + 1], data[i + 1], resolver);
    }
    return { output };
}

}