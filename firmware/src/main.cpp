//
// Created by Benjamin Skirlo on 14/12/16.
//

#include <Arduino.h>
#include <SerialCommand.h>
#include <Base64.h>
#include "TFT.h"
#include <Button.h>



#define notImplemented Serial.print("function not implemented"); return;

#define UPP_ACKNOWLEDGE      "UPP-ACK"
#define UPP_ERROR            "UPP-ERR"
#define UPP_HUB_HANDSHAKE    "UPP-HUB"
#define UPP_FILTER_HANDSHAKE "UPP-FILTER"
#define UPP_INIT_END         "UPP-READY"
/**
 * UPP-LIST [name :String] [length :Number]
 */
#define UPP_LIST             "UPP-LIST"
#define UPP_LIST_END         "UPP-END"
/**
 * UPP-ENTRY [name :String] [pictogram :Base64]
 */
#define UPP_ENTRY            "UPP-ENTRY"
#define UPP_SELECT_SENSOR    "UPP-SELECT"

#define KNOB_PIN       A0
#define KNOB_THRESHOLD 10
#define BUTTON_PIN      2

int _logI = 0;
#define _logLevel 0


template<typename T>
void log(int level, T msg){ //TODO:rewrite to macro
    if(level > _logLevel) return;
    Serial.print("log[");
    Serial.print(++_logI);
    Serial.print(":");
    Serial.print(_logLevel);
    Serial.print("]: ");
    Serial.print(msg);
    Serial.println();
}

enum State{
    STARTUP_STATE,
    HANDSHAKE_STATE,
    RECEIVING_SENSOR_LIST_STATE,
    DISPLAY_SENSORS_STATE,
    RECEIVE_INITIAL_DATA_STATE,
    RECEIVE_LIST_STATE,
    RECEIVE_LIST_END_STATE,
    LENGTH_OF_STATES
};





SerialCommand SCmd;
Button button(BUTTON_PIN, true, true, 20);

enum List{
    NO_LIST,
    SENSOR_LIST,
    LENGTH_OF_LISTS
};

struct Sensor{
    int      id;
    String   name;
    char   * graphic;
};

struct Store {
    Store *  old_store        = 0;
    int      __depth          = 0;
    bool     dirty            = true;
    bool     hub_acknowledged = false;
    State    state            = State::STARTUP_STATE;
    Sensor * sensors;
    int      sensors_list_length    = 0;
    int      received_from_list     = 0;
    int      selected_sensor        = 0;
    int      knobValue              = 0;
    bool     knobValueUnconsumed    = false;
    bool     buttonPressed          = false;
    bool     buttonChangeUnconsumed = false;
} __store;

void debug_store(Store _store = __store){
    log(2,"__depth         :");
    log(2,_store.__depth);
    log(2,"dirty           :");
    log(2,_store.dirty);
    log(2,"hub_acknowledged:");
    log(2,_store.hub_acknowledged);
    log(2,"state           :");
    log(2,_store.state);


//    if(_store.__depth > 0) {
//        Serial.println("");
//        Serial.println("before: ");
//        debug_store(*_store.old_store);
//    }
}


struct Action{ };

int is_acknowledged(const Store &store) {
    if(!store.hub_acknowledged){
        log(0,"Hub is not acknowledged");
        return 1;
    }
    return 0;
}

int is_right_state(const Store &store, State state) {
    if(store.state != state){
        log(0, "Currently in the wrong state");
        return 1;
    }
    return 0;
}

template <typename T>
void rx(T action){
    Store store(__store);
    store.__depth += 1;
//    store->old_store = __store;
    Store newStore = rx(action, store);
    __store = newStore;
}

struct NoAction               :Action {};
struct AcknowledgeAction      :Action {};
struct HandshakeAction        :Action {};
struct StartHandshakeAction   :Action {};
struct EvaluateAction         :Action {};
struct EndInitialStateAction  :Action {};
struct ReceiveListAction      :Action {};
struct ReceiveListEntryAction :Action {};
struct ReceiveListEndAction   :Action {};
struct PollInterface          :Action {};
struct SelectSensorAction     :Action {};
struct RequestSensorData      :Action {};

void setupUpp();

void tx(String message, bool awaiting_acknowledgement = false){
    if(awaiting_acknowledgement)rx(EvaluateAction());
    Serial.print(message);
    Serial.println();
}

void generate_sensor_select_menu(char * textBuffer, int page, Store store){
    int list_length = 3;
    int selected_sensor = store.selected_sensor;
    int page_begin = page * list_length;
    int len = sprintf(textBuffer, "Select Sensor\n[%s]%s\n[%s]%s\n[%s]%s\n\n%d%d",
          store.sensors_list_length > page_begin + 0
          ? (selected_sensor == page_begin + 0 ? "X" : " ")
          : "-",
          store.sensors_list_length > page_begin + 0
          ? store.sensors[page_begin + 0].name.c_str()
          : "-",
          store.sensors_list_length > page_begin + 1
          ? (selected_sensor == page_begin + 1 ? "X" : " ")
          : "-",
          store.sensors_list_length > page_begin + 1
          ? store.sensors[page_begin + 1].name.c_str()
          : "-",
          store.sensors_list_length > page_begin + 2
          ? (selected_sensor == page_begin + 2 ? "X" : " ")
          : "-",
          store.sensors_list_length > page_begin + 2
          ? store.sensors[page_begin + 2].name.c_str()
          : "-",
          selected_sensor,
          store.buttonPressed
    );

    textBuffer[len] = 0;
}

Store rx(EvaluateAction _, Store store){
    char textBuffer [120];
    int  page        = 0;

    log(2, "Display is dirty");
    store.dirty = false;
    switch (store.state){
        case State::DISPLAY_SENSORS_STATE:


            generate_sensor_select_menu(textBuffer, page, store);
            writeToTFT(textBuffer, 2);

            break;
        default:
            writeToTFT("Conecting", 3);
            break;
    }


    return store;
}

Store rx(Action _, Store newStore){
    log(0, "No operation");
    return newStore;
}

Store rx(AcknowledgeAction _, Store newStore){
    newStore.hub_acknowledged = true;

    return newStore;
}

Store rx(HandshakeAction _, Store newStore){
    int failure = 0;
    if(newStore.state !=  State::HANDSHAKE_STATE){
        log(0, "expected to be in HANDSHAKE_STATE");
        failure ++;
    }

    if(!failure){
        log(1, "Handshake successful");
        tx(UPP_ACKNOWLEDGE);
        newStore.state = State::RECEIVE_INITIAL_DATA_STATE;
    }else{
        log(0, "Unsuccessful Handshake. FailureLevel: ");
        log(0, failure);
        tx(UPP_ERROR);
    }
    return newStore;
}

Store rx(StartHandshakeAction _, Store newStore){
    if(!newStore.hub_acknowledged) {
        tx(UPP_FILTER_HANDSHAKE);
    }else{
        log(1, "Transition to HANDSHAKE_STATE");
        newStore.state = State::HANDSHAKE_STATE;
        debug_store();
    }

    return newStore;
}

Store rx(ReceiveListAction _, Store store){
    auto failure = 0;
    auto name     = SCmd.next();
    auto lengthSt = SCmd.next();
    auto length = atoi(lengthSt);

    List list = List::NO_LIST;

    failure += is_acknowledged(store);
    failure += is_right_state(store, State::RECEIVE_INITIAL_DATA_STATE);



    if(strlen(lengthSt) == 0 || length == 0){
        log(0, "Length of list is unknown");
        failure ++;
    }

    if(strcmp("SENSOR", name) == 0){
        list = List::SENSOR_LIST;
    }

    if(list == List::NO_LIST){
        log(0, "Unknown List");
        failure ++;
    }

    if(failure){
        tx(UPP_ERROR);
        return store;
    }

    log(1, "Awaiting entries");
    log(1, "name:");
    log(1, name);
    log(1, "Length:");
    log(1, length);
    log(1, "Change to State: RECEIVE_LIST_STATE");

    store.sensors = new Sensor[length];
    store.sensors_list_length = length;
    store.received_from_list  = 0;
    store.state   = State::RECEIVE_LIST_STATE;

    tx(UPP_ACKNOWLEDGE);

    return store;
}


Store rx(ReceiveListEntryAction _, Store store){
    auto failure = 0;

    auto idString     = SCmd.next();
    auto name         = SCmd.next();
    auto pictogramB64 = SCmd.next();
    auto * pictogram  = new char[256 / 8];
    auto id           = atoi(idString);

    switch(store.state){
        case State::RECEIVE_LIST_STATE:
            break;
        default:
            log(0, "In wrong state to Receive List entry");
            failure ++;
            break;
    }

    if(strlen(pictogramB64) == 0){
        log(0, "can't decode Entry");
        failure++;
    }else{
        Base64.decode(pictogram, pictogramB64, strlen(pictogramB64));
    }

    if(failure){
        tx(UPP_ERROR);
        return store;
    }

    Sensor sensor;
    sensor.id      = id;
    sensor.name    = name;
    sensor.graphic = pictogram;

    log(1, "Awaiting entries");
    log(1, "name:");
    log(1, name);
    log(1, "number:");
    log(1, store.received_from_list);
    log(1, "pictogram:");
    log(1, pictogram);

    store.sensors[store.received_from_list] = sensor;
    store.received_from_list++;

    if(store.received_from_list >= store.sensors_list_length){
        log(1, "Change to State: RECEIVE_LIST_END_STATE");
        store.state = State::RECEIVE_LIST_END_STATE;
    }

    tx(UPP_ACKNOWLEDGE);

    return store;
}


Store rx(ReceiveListEndAction _, Store store) {
    auto failure = 0;
    switch(store.state){
        case State::RECEIVE_LIST_END_STATE:
            break;
        default:
            log(0, "In wrong state to Receive List end");
            failure ++;
            break;
    }

    if(failure){
        tx(UPP_ERROR);
        return store;
    }

    log(1, "Change to State: RECEIVE_INITIAL_DATA_STATE");
    store.state = State::RECEIVE_INITIAL_DATA_STATE;
    store.dirty = true;
    tx(UPP_ACKNOWLEDGE);
    return store;
}

Store rx(EndInitialStateAction _, Store store){
    int failure = 0;
    failure += is_right_state(store, State::RECEIVE_INITIAL_DATA_STATE);

    if(failure){
        tx(UPP_ERROR);
        return store;
    }

    log(1, "Change to State: DISPLAY_SENSORS_STATE");
    store.state = State::DISPLAY_SENSORS_STATE;
    store.dirty = true;
    tx(UPP_ACKNOWLEDGE);

    return store;

}

Store rx(PollInterface _, Store store){
    button.read();
    int knobValue = analogRead(KNOB_PIN);
    if(abs(knobValue - store.knobValue) > KNOB_THRESHOLD){
        store.knobValue           = knobValue;
        store.knobValueUnconsumed = true;
    }

    if(button.wasReleased()){
        store.buttonPressed          = true;
        store.buttonChangeUnconsumed = true;
        store.dirty                  = true;
    }

    return store;
}

Store rx(SelectSensorAction _, Store store){
    auto selected_sensor = map(
            store.knobValue,
            0, 1024,                      //from
            0, store.sensors_list_length  //to
    );
    if(store.selected_sensor != selected_sensor)
        store.dirty = true;

    store.selected_sensor     = selected_sensor;
    store.knobValueUnconsumed = false;

    return store;
}

Store rx(RequestSensorData _, Store store){
    char buffer[50];
    auto sensor = store.sensors[store.selected_sensor];
    auto id     = sensor.id;

    if(!store.buttonPressed)
        return store;

    sprintf(buffer, "%s %d", UPP_SELECT_SENSOR, id);
    tx(buffer);

    store.buttonChangeUnconsumed = false;
    
    
    return store;
}

void inline setupInterface(){
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    setupTFT();
}

void setupUpp() {
    Serial.begin(9600);

    SCmd.addCommand(UPP_ACKNOWLEDGE  , [](){rx(AcknowledgeAction     ());});
    SCmd.addCommand(UPP_HUB_HANDSHAKE, [](){rx(HandshakeAction       ());});
    SCmd.addCommand(UPP_HUB_HANDSHAKE, [](){rx(HandshakeAction       ());});
    SCmd.addCommand(UPP_LIST         , [](){rx(ReceiveListAction     ());});
    SCmd.addCommand(UPP_ENTRY        , [](){rx(ReceiveListEntryAction());});
    SCmd.addCommand(UPP_LIST_END     , [](){rx(ReceiveListEndAction  ());});
    SCmd.addCommand(UPP_INIT_END     , [](){rx(EndInitialStateAction ());});
    SCmd.setDefaultHandler([](const char * c){tx(UPP_ERROR);log(0,"Unknown Command");log(0, c);});
}

void setup(){
    setupUpp();
    setupInterface();
}



void loop(){
//    debug_store();
    rx(PollInterface());
    if(__store.dirty){
        rx(EvaluateAction());
    }
    switch (__store.state){
        case State::STARTUP_STATE:
            rx(StartHandshakeAction());
            break;
        case State::DISPLAY_SENSORS_STATE:
            if(__store.knobValueUnconsumed)
                rx(SelectSensorAction());
            if(__store.buttonChangeUnconsumed)
                rx(RequestSensorData());
            break;
//        case State::RECEIVING_SENSOR_LIST_STATE:
//            //dummy transmission indicator
//            break;
    }

    delay(50);
    SCmd.readSerial();
}

