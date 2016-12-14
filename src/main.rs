/*
 * This programm is managing the communication to a single Filter Node
 */
extern crate clap;
extern crate serial;

use std::io;
use std::time::Duration;

use clap::{Arg, App};

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

    println!("Send: {:?}{:?}", st, "\r\n")
}

fn await_line<T: serial::SerialPort>(port: &mut T, st: String) -> Result<(), &str>{
    let search_line = st + "\r\n";
    let line :String = match read_line(port) {
        Some(input) => Ok(input),
        _           => Err("No Connection")
    }?;

    if line.eq(&search_line) {Ok(())} else {Err("No reply")}
}

fn handshake<T: serial::SerialPort> (port: &mut T) -> Result<FilterNode, &str>{
    let node  :FilterNode;

    let line :String = match read_line(port) {
        Some(input) => Ok(input),
        _           => Err("No Connection")
    }?;

    let node = match &line[..]{
        "UPP-FILTER\r\n" => Ok(FilterNode{name: "hallo".to_string()}),
        _              => Err("Unknown Device")
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

fn send_acknowledge<T: serial::SerialPort>(port: &mut T){
    println!("Send Acknowledge");
    send_line(port, "UPP-ACK".to_string());
}

fn main() {

    let matches = process_arguments();
    let device  = matches.value_of("device").unwrap();
    println!("Open {:?}", device);

    let mut port = serial::open(device).unwrap();

    let node :FilterNode = match pollHandshake(&mut port, 0, 10){
        Some(node) => node,
        None => return
    };

    send_acknowledge(&mut port);

    std::thread::sleep_ms(50);

    println!("Send Handshake");
    send_line(&mut port, "UPP-HUB".to_string());


    println!("Wait for Acknowledge");
    match await_line(&mut port, "UPP-ACK".to_string()){
        Ok(_)  => println!("Acknowledged"),
        Err(_) => println!("Unknown"),
    }
}
