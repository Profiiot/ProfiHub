/*
 * This programm is managing the communication to a single Filter Node
 */
extern crate clap;
extern crate serial;
extern crate regex;
extern crate base64;

use std::io;
use std::time::Duration;
use regex::Regex;
use regex::Captures;
use regex::CaptureNames;
use std::collections::HashMap;
use std::collections::VecDeque;
use std::collections::hash_map::Iter;

use clap::{Arg, App};

fn load_upp_commands() -> HashMap<String, Regex>{
    let mut commands :HashMap<String, Regex> = HashMap::new();
    commands.insert(
        "UPP-FILTER".to_string(),
        Regex::new(r"^UPP-FILTER v(?P<version>[:digit:]{3}) (?P<name>[:alphanum:]*)").unwrap()
    );

    commands.insert(
        "UPP-SELECT".to_string(),
        Regex::new(r"^UPP-SELECT (?P<sensor_id>[:digit:]*)").unwrap()
    );

    commands.insert(
        "UPP-ACK".to_string(),
        Regex::new(r"^UPP-ACK (?P<state>.*)").unwrap()
    );

    commands
}

struct Sensor{
    id        :i32,
    name      :String,
    pictogram :String,
}

fn load_sensors() -> HashMap<String, Sensor>{
    let mut sensors :HashMap<String, Sensor> = HashMap::new();
    let potentiometer = Sensor{
        id:        1,
        name:      "potentiometer".to_string(),
        pictogram: "HIJKL".to_string()
    };

    let light_sensor = Sensor{
        id:        2,
        name:      "light".to_string(),
        pictogram: "ABCDEF".to_string()
    };

    sensors.insert(potentiometer.name.clone(), potentiometer);
    sensors.insert(light_sensor .name.clone(), light_sensor);

    sensors
}

fn connect<T: serial::SerialPort> (port: &mut T) -> io::Result<()>{
    port.reconfigure(&|settings| {
        (settings.set_baud_rate(serial::Baud115200))?;
        settings.set_char_size(serial::Bits8);
        settings.set_parity(serial::ParityNone);
        settings.set_stop_bits(serial::Stop1);
        settings.set_flow_control(serial::FlowNone);
        Ok(())
    })?;

    port.set_timeout(Duration::from_millis(5000))?;
    Ok(())
}

fn read_line<T: serial::SerialPort> (port: &mut T) -> Option<String>{
    let mut received_string: String = String::new();
    loop {
        let buf: &mut [u8; 1] = &mut [0u8];

        if port.read(buf).is_ok() {
            received_string.push(buf[0] as char);
            if buf[0] == '\n' as u8 {
                break;
            };
        }
    }
    println!("received string: {:?}", received_string);
    Some(received_string)
}

struct FilterNode {
    name :String,
}

fn send_line(port: &mut serial::SerialPort, st: String){
    port.write(st.as_bytes());
    port.write(b"\r\n");
    std::thread::sleep_ms(100);

    println!("Send: {:?}{:?}", st, "\r\n")
}

fn await_line<T: serial::SerialPort>(port: &mut T, st: String) -> Result<(), &str>{
    let search_line = st + "\r\n";
    let line :String = match read_line(port) {
        Some(input) => Ok(input),
                  _ => Err("No Connection")
    }?;

    if line.eq(&search_line) {Ok(())} else {Err("No reply")}
}

fn await_command<T: serial::SerialPort, >(port: &mut T, match_expression: Regex) -> Option<HashMap<String, String>> {
    let line :String = match read_line(port) {
        Some(v) => v,
              _ => return None,
    };
    ;
    let mut values   :HashMap<String, String> = HashMap::new();
    let     captures :Captures = match match_expression.captures(line.as_str()){
        Some(v) => v,
              _ => {return None}
    };


    for capture in captures.iter_named() {
        let name  :String = capture.0.to_string();
        let value :String = match capture.1 {
            Some(v) => v.to_string(),
                  _ => "".to_string()
        };
        values.insert(name, value);
    }

    Some(values)
}

fn await_acknowledge<T: serial::SerialPort>(port: &mut T) -> Result<(), &str>{
    await_line(port, "UPP-ACK".to_string())
//        {
//        Ok(_)  => {println!("Acknowledged"); Ok(())},
//        Err(e) => {println!("Failure in communication"); Err(e)},
//    }
}

fn handshake<T: serial::SerialPort> (port: &mut T) -> Result<FilterNode, &str>{
    let node  :FilterNode;

    let line :String = match read_line(port) {
        Some(input) => Ok(input),
                  _ => Err("No Connection")
    }?;

    let node = match &line[..]{
        "UPP-FILTER\r\n" => Ok(FilterNode{name: "hallo".to_string()}),
                       _ => Err("Unknown Device")
    }?;

    Ok(node)


}

fn process_arguments<'T>() -> clap::ArgMatches<'T>{
    let matches = App::new("UPP Interface")
        .version("0.01")
        .author("Benjamin S. <benjamin.skirlo@ixds.com>")
        .arg(Arg::from_usage("<device>")
            .value_name("device Path")
            .required(true)
        )
        .arg(Arg::with_name("platformio-bin")
            .help("path to platformio binary")
        )
        .get_matches();

    matches
}

fn pollHandshake<T: serial::SerialPort>(port: &mut T, trial :i32, trials: i32) -> Option<FilterNode>{
    println!("Try to read Handshake: trial {:?} of {:?}", trial, trials);
    match handshake(port){
        Ok(node) => Some(node),
        Err(_)   => {
            if trial < trials {
                pollHandshake(port, trial + 1, trials)
            } else {None}
        }
    }
}

fn send_acknowledge<T: serial::SerialPort>(port: &mut T) {
    println!("Send Acknowledge");
    send_line(port, "UPP-ACK".to_string());
}

fn clean_reading<T: serial::SerialPort>(port: &mut T) {
    let mut t_buffer :Vec<u8> = Vec::new();
    port.read(&mut t_buffer);
}

fn sendList<Tp: serial::SerialPort> (port: &mut Tp, values :VecDeque<String>, name :String){
    let length :usize = values.len();

    send_line(port, format!("UPP-LIST {} {}", name, length).to_string());
    assert!(await_acknowledge(port).is_ok());


    for value in values {

        send_line(port, value);
        assert!(await_acknowledge(port).is_ok());
    }

    send_line(port, "UPP-END".to_string());
    assert!(await_acknowledge(port).is_ok())
}

fn handshake_and_select_sensor<'a>(device :&str, sensors: &'a HashMap<String, Sensor>) -> Option<& 'a Sensor>{
    let commands :HashMap<String, Regex> = load_upp_commands();
    let mut port = serial::open(device).unwrap();
    //    std::thread::sleep_ms(5000);

    clean_reading(&mut port);


    let node: FilterNode = match pollHandshake(&mut port, 0, 10) {
        Some(node) => node,
        None => return None
    };

    send_acknowledge(&mut port);


    println!("Send Handshake");
    send_line(&mut port, "UPP-HUB".to_string());

    println!("Wait for Acknowledge");



    let sensor_lines: VecDeque<String> = sensors.iter().map(
        |sensor: (&String, &Sensor)|
            format!("UPP-ENTRY {} {} {}", sensor.1.id, sensor.0, sensor.1.pictogram)
    ).collect();


    sendList(&mut port, sensor_lines, "SENSOR".to_string());

    send_line(&mut port, "UPP-READY".to_string());
    assert!(await_acknowledge(&mut port).is_ok());

    await_command(&mut port, commands.get("UPP-ACK").unwrap().clone());

    let sensor_id: String = match await_command(&mut port, commands.get("UPP-SELECT").unwrap().clone()) {
        Some(v) => v.get("sensor_id").unwrap().clone(),
        _       => return None,
    };

    let sensor: &Sensor = match sensors.iter().find(|&_sensor| _sensor.1.id.to_string() == sensor_id) {
        Some(v) => v.clone().1.clone(),
        _       => return None,
    };

    println!("{:?}", sensor.name);
    Some(sensor)
}

fn main() {
    let commands :HashMap<String, Regex> = load_upp_commands();
    let matches  = process_arguments();
    let device   = matches.value_of("device").unwrap();
    let sensors: HashMap<String, Sensor> = load_sensors();

    println!("Open {:?}", device);

    let sensor :&Sensor = handshake_and_select_sensor(device, &sensors).unwrap();

    let sed          = "/usr/bin/sed";
    let replace      = format!("###_LIB_PATH_###/{}\\/{}.ino", sensor.name, sensor.name);
    let firmware_src = "firmware/src/sensorsLib/";


    let sed_output = std::process::Command::new("sh")
        .arg("-c")
        .arg(format!("{} 's/{}/g' {}libIncludeTemplate.h > {}libInclude.h", sed, replace, firmware_src, firmware_src))
        .output()
        .expect("didn't start");

    println!("Replaced include: {:?}", sed_output);

    let pio_output = std::process::Command::new("sh")
        .arg("-c")
        .arg("/usr/local/bin/platformio run -d firmware --upload-port /dev/cu.usbmodem1411 -t upload")
        .status()
        .expect("didn't start");
    println!("Programmed: {:?}", pio_output);
//    println!("Programmed");
}
