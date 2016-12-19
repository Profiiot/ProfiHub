//
// Created by Benjamin Skirlo on 14/12/16.
//

#include <Arduino.h>
#include <SerialCommand.h>
#include <Base64.h>

SerialCommand SCmd;

#define notImplemented Serial.print("function not implemented"); return;

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


#define UPP_ACKNOWLEDGE      "UPP-ACK"
#define UPP_ERROR            "UPP-ERR"
#define UPP_HUB_HANDSHAKE    "UPP-HUB"
#define UPP_FILTER_HANDSHAKE "UPP-FILTER"
/**
 * UPP-LIST [name :String] [length :Number]
 */
#define UPP_LIST             "UPP-LIST"
#define UPP_LIST_END         "UPP-LIST-END"
/**
 * UPP-ENTRY [name :String] [pictogram :Base64]
 */
#define UPP_ENTRY            "UPP-ENTRY"

enum List{
    NO_LIST,
    SENSOR_LIST,
    LENGTH_OF_LISTS
};

struct Sensor{
    String   name;
    char   * graphic;
};

struct Store {
    Store *  old_store       = 0;
    int      __depth          = 0;
    bool     dirty            = false;
    bool     hub_acknowledged = false;
    State    state            = State::STARTUP_STATE;
    Sensor * sensors;
    int      sensors_list_length = 0;
    int      received_from_list  = 0;
} __store;

void debug_store(Store _store = __store){
    log(2,"__depth         :");
    log(2,_store.__depth);
    log(2,"dirty           :");
    log(2,_store.dirty);
    log(2,"hub_acknowledged:");
    log(2,_store.hub_acknowledged);
    log(2,"state:           ");
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
struct ReceiveListAction      :Action {};
struct ReceiveListEntryAction :Action {};
struct ReceiveListEndAction   :Action {};

void tx(String message, bool awaiting_acknowledgement = false){
    if(awaiting_acknowledgement)rx(EvaluateAction());
    Serial.print(message);
    Serial.println();
}

Store rx(EvaluateAction _, Store store){
    store.hub_acknowledged = false;
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

    auto name         = SCmd.next();
    auto pictogramB64 = SCmd.next();
    char * pictogram  = new char[256 / 8];

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
    sensor.name    = name;
    sensor.graphic = pictogram;

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
    log(0, "In wrong state to Receive List end");
    switch(store.state){
        case State::RECEIVE_LIST_END_STATE:
            break;
        default:
            log(0, "In wrong state to Receive List end");
            failure ++;
            break;
    }

    if(failure){
        log(0, "In wrong state to Receive List end");
        tx(UPP_ERROR);
        return store;
    }

    log(1, "Change to State: RECEIVE_INITIAL_DATA_STATE");
    store.state = State::RECEIVE_INITIAL_DATA_STATE;

    tx(UPP_ACKNOWLEDGE);
    return store;
}


void setup(){
    Serial.begin(9600);

    SCmd.addCommand(UPP_ACKNOWLEDGE  , [](){rx(AcknowledgeAction     ());});
    SCmd.addCommand(UPP_HUB_HANDSHAKE, [](){rx(HandshakeAction       ());});
    SCmd.addCommand(UPP_HUB_HANDSHAKE, [](){rx(HandshakeAction       ());});
    SCmd.addCommand(UPP_LIST         , [](){rx(ReceiveListAction     ());});
    SCmd.addCommand(UPP_ENTRY        , [](){rx(ReceiveListEntryAction());});
//    SCmd.addCommand(UPP_LIST_END     , [](){rx(ReceiveListEndAction  ());});
    SCmd.setDefaultHandler([](const char * c){log(0,"Unknown Command");log(0, c);});
}

void loop(){
//    debug_store();
    switch (__store.state){
        case State::STARTUP_STATE:
            rx(StartHandshakeAction());
            break;
//        case State::RECEIVE_INITIAL_DATA_STATE:
//            dummy;
            break;
//        case State::RECEIVING_SENSOR_LIST_STATE:
//            //dummy transmission indicator
//            break;
    }
    delay(50);
    SCmd.readSerial();
}

