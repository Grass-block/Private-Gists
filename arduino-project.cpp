
//串口调试工具封装
namespace Debug{
    void start(int rate){
        Serial.begin(rate);
    }

    void printLine(String content){
        Serial.print(content+"\n");
    }

    void printLine(float content){
        Serial.print(content);
        Serial.print("\n");
    }

    void init(){
        Serial.begin(9600);
    }
}

//任务管理
namespace TaskManager {
    //callback function type decleared to 'function(int)'.
    typedef void (*Callback)(int);

    const char TASK_POOL_CAP = 64;
    const char EMPTY_SEGMENT = -127;
    const char RUN_INFINITE = -1;

    Callback functions[TASK_POOL_CAP];
    char intervals[TASK_POOL_CAP];
    int times[TASK_POOL_CAP];
    int operations[TASK_POOL_CAP];
    int ticks = 0;

    //makesure your taskId is inbound and will not fuck the array up.
    bool ensureLimit(int id) {
        return id < 0 || id >= TASK_POOL_CAP;
    }

    //add a callback function(or register maybe).
    void add(Callback callback, int id, int interval,int remain) {
        if (ensureLimit(id)) {
            return;
        }

        functions[id] = callback;
        intervals[id] = interval;
        times[id]=remain;
        operations[id] = 0;
    }

    //remove a task
    void cancel(int id) {
        if (ensureLimit(id)) {
            return;
        }
        intervals[id] = EMPTY_SEGMENT;
    }

    //iterate all functions and run them.
    void tick() {
        ticks++;
        for (char i = 0; i < TASK_POOL_CAP; i++) {
            if (intervals[i] == EMPTY_SEGMENT) {
                continue;
            }
            if (ticks % intervals[i] != 0) {
                continue;
            }

            functions[i](operations[i]);
            operations[i]++;

            if (times[i] == RUN_INFINITE) {
                continue;
            }
            times[i]--;
            if (times[i] <= 0) {
                cancel(i);
            }
        }
    }

    //get how much the tick is passed.
    int currentTick() {
        return ticks;
    }

    //get a registered function.
    Callback get(int id) {
        if (intervals[id] == EMPTY_SEGMENT) {
            return nullptr;
        }
        return functions[id];
    }
}

//delegation for port query
//usage of functions are same as TaskManager
//接口信号主动查询和主动反触发事件
namespace PortEvent {
    //todo: check the query logic
    //todo: deprecate the namespace "Debug"

    //callback function type decleared to 'function(int)' (int for port value).
    typedef void (*Callback)(int);
        void __empty__() {

    }

    const char TASK_POOL_CAP = 32;
    const Callback EMPTY_FUNCTION = __empty__;

    Callback functions[TASK_POOL_CAP];
    //analog id will be added TASK_POOL_CAP,
    //so subtract it we could get raw port value and also make sure which listen type is that.
    int queryPorts[TASK_POOL_CAP];
    int lastVal[TASK_POOL_CAP];

    bool ensureLimit(int id) {
        return id < 0 || id >= TASK_POOL_CAP;
    }

    void add(Callback callback, int id, int listen, bool analog) {
        if (ensureLimit(id)) {
            return;
        }

        functions[id] = callback;
        if (analog) {
            queryPorts[id] = listen + TASK_POOL_CAP;
        }else {
            queryPorts[id] = listen;
        }
    }

    void tick() {
        for (char i = 0; i < TASK_POOL_CAP; i++) {
            if (functions[i] == EMPTY_FUNCTION) {
                continue;
            }

            int val = 0;
            int fixedPort = queryPorts[i] - TASK_POOL_CAP;


            if (fixedPort < 0) {
                fixedPort = queryPorts[i];
                val = analogRead(fixedPort);
            }else {

                val = digitalRead(fixedPort);
            }

            //yep it's a "change listener"
            if (val != lastVal[i]) {
                Serial.print(val);
                lastVal[i] = val;
                functions[i](val);
            }
        }
    }
}


//引用程序逻辑实现部分
namespace Application {
    const int LISTENER_DOOR_ID=12;
    const int LISTENER_BTN_ID=13;
    const int LISTENER_ID=16;
    const int SOUND_HANDLER = 13;
    const int TASK_SPLASH = 17;
    const int TASK_LIGHT = 18;

    void splash(int tick){
        digitalWrite(7,tick%2);
    }

    void action(int value) {
        if (value != 0) {
            return;
        }
        TaskManager::add(splash,TASK_SPLASH,20,TaskManager::RUN_INFINITE);
    }

    void cancelAction(int value){
        if (value != 0) {
            return;
        }
        TaskManager::cancel(TASK_SPLASH);
    }

    void soundLight(int tick){
        if(tick<2000){
            digitalWrite(4,1);
            return;
        }
        digitalWrite(4,0);
        TaskManager::cancel(TASK_LIGHT);
    }

    void sound(int value){
        if(value > 127.0){
           TaskManager::add(soundLight,TASK_LIGHT,40,TaskManager::RUN_INFINITE);
        }
    }

    void test(int val){
        if(val==0){
            TaskManager::add(splash,TASK_SPLASH,200,TaskManager::RUN_INFINITE);
        }
    }

    void init() {
       PortEvent::add(test,LISTENER_DOOR_ID, 2, false);
    }

    void tick() {
    }
}


//arduino 内置函数转接器
void setup() {
    Serial.begin(9600);
    Application::init();
}

void loop() {
    TaskManager::tick();
    PortEvent::tick();
    Application::tick();
}
