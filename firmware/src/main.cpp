//
// Created by Benjamin Skirlo on 14/12/16.
//

#include <Arduino.h>
#include <SerialCommand.h>
#define SERIALCOMMAND_DEBUG
SerialCommand SCmd;

#define notImplemented Serial.print("function not implemented"); return;

int _logI = 0;
#define _logLevel 0
template<typename T>
void log(int level, T msg){
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
};


#define UPP_ACKNOWLEDGE      "UPP-ACK"
#define UPP_ERROR            "UPP-ERR"
#define UPP_HUB_HANDSHAKE    "UPP-HUB"
#define UPP_FILTER_HANDSHAKE "UPP-FILTER"

struct Store {
    Store * old_store       = 0;
    int    __depth          = 0;
    bool   dirty            = false;
    bool   hub_acknowledged = false;
    State  state            = State::STARTUP_STATE;
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

struct NoAction             :Action {};
struct AcknowledgeAction    :Action {};
struct HandshakeAction      :Action {};
struct StartHandshakeAction :Action {};
struct EvaluateAction       :Action {};




template <typename T>
void rx(T action){
    Store store(__store);
    store.__depth += 1;
//    store->old_store = __store;
    Store newStore = rx(action, store);
    __store = newStore;
}

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
        newStore.state = State::DISPLAY_SENSORS_STATE;
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

void setup(){
    Serial.begin(9600);

    SCmd.addCommand(UPP_ACKNOWLEDGE  , [](){rx(AcknowledgeAction());});
    SCmd.addCommand(UPP_HUB_HANDSHAKE, [](){rx(HandshakeAction  ());});
}

void loop(){
//    debug_store();
    switch (__store.state){
        case State::STARTUP_STATE:
            rx(StartHandshakeAction());
            break;
//        case State::DISPLAY_SENSORS_STATE:
//            //dummy;
//            break;
//        case State::RECEIVING_SENSOR_LIST_STATE:
//            //dummy transmission indicator
//            break;
    }
    delay(50);
    SCmd.readSerial();
}

