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
        (settings.set_baud_rate(serial::Baud9600))?;
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

    Some(received_string)
}

struct FilterNode {
    name :String,
}

fn send_line(port: &mut serial::SerialPort, st: String){
    port.write(st.as_bytes());
    port.write(b"\n\r");
}

fn handshake<T: serial::SerialPort> (port: &mut T) -> Result<FilterNode, &str>{
    let node  :FilterNode;

    let line = match read_line(port) {
        Some(input) => Ok(input),
        _           => Err("No Connection")
    }?;

    let is_known = match &line[..]{
        "UPP-HANDSHAKEM0\r\n" => Ok(FilterNode{name: "hallo".to_string()}),
        _                     => Err("Unknown Device")
    };

    node = is_known?;
    send_line(port, line);

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

fn pollHandshake<T: serial::SerialDevice>(port: &mut T, trial :i32, trials: i32) -> Option<FilterNode>{
    match handshake(port){
        Ok(node) => Some(node),
        Err(_)   => {
            if trials < trial {
                pollHandshake(port, trial + 1, trials)
            } else {None}}
    }
}

fn main() {

    let matches = process_arguments();
    let device  = matches.value_of("device").unwrap();
    println!("Open {:?}", device);

    let mut port = serial::open(device).unwrap();
    let node     = handshake(&mut port).unwrap();


}
