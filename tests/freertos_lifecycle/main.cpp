#include "Arduino.h"
void test_yield(){std::this_thread::yield();}
#include "ThingerESP32FreeRTOS.h"
#include "common.h"
int main(){
    Client wire;ThingerClient client(wire,"u","d","c");
    ThingerESP32FreeRTOS runner(client);
    // Stop before the task has first been scheduled.
    hold_entry=true;assert(runner.start());assert(!runner.start());
    auto stopping=std::async(std::launch::async,[&]{return runner.stop();});
    wait_until([&]{return !runner.is_running();});
    assert(stopping.wait_for(std::chrono::milliseconds(10))==std::future_status::timeout);
    assert(!runner.stop(2)); // The other caller holds the lifecycle mutex.
    hold_entry=false;assert(stopping.get());join_tasks();
    assert(returned==0 && self_deleted==1);
    // External stop must wait until an in-flight client operation finishes.
    std::atomic<bool> entered{false},release{false};
    wire.on_available=[&]{entered=true;while(!release.load())vTaskDelay(1);};
    assert(runner.start());wait_until([&]{return entered.load();});
    auto waiting=std::async(std::launch::async,[&]{return runner.stop();});
    wait_until([&]{return !runner.is_running();});
    assert(waiting.wait_for(std::chrono::milliseconds(10))==std::future_status::timeout);
    release=true;assert(waiting.get());join_tasks();
    assert(returned==0 && self_deleted==2);
    // A callback can request its own stop without joining/deleting itself mid-call.
    entered=false;
    wire.on_available=[&]{assert(runner.stop());assert(!runner.start());entered=true;};
    assert(runner.start());wait_until([&]{return entered.load();});
    assert(runner.stop());join_tasks();assert(returned==0 && self_deleted==3);
    wire.on_available=nullptr;
    hold_entry=true;assert(runner.start());
    assert(!runner.stop(2)); // The worker itself has not been scheduled.
    hold_entry=false;assert(runner.stop());join_tasks();
    fail_create=true;assert(!runner.start());assert(!runner.is_running());assert(runner.stop());
    assert(runner.start());assert(runner.stop());join_tasks();
    assert(returned==0 && self_deleted==5);
    fail_semaphore=true;
    {ThingerESP32FreeRTOS allocation_failure(client);assert(!allocation_failure.start());assert(allocation_failure.stop());}
    assert(runner.stop()); // Repeated stop is harmless.
    std::cout<<"FreeRTOS lifecycle regression passed\n";
}
