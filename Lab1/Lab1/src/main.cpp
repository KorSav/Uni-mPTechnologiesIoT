#include "hardware.hpp"
#include "System.hpp"

int main() {
    TIMER_Init_1ms();

    LedDriver ledDriver(led);

    System system(relayOpen, relayClose, buzzer, ledDriver);
    system.init(msTicks);

    btnOpen.setCallback([&](){ system.onOpen(); });
    btnClose.setCallback([&](){ system.onClose(); });
    moveDetector.setCallback([&](){ system.onMotion(); });
    btnService.setCallback([&](){ system.onService(); });

    while(true) {
        system.update(msTicks);
        __WFI();
    }
}