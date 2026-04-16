#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

/// Function prototype for sending data to ThingsBoard via HTTP POST request
void send_to_http(float voltage, float current, float power, float energy); 

#endif