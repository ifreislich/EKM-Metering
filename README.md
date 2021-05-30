# EKM-Metering

I wrote this in 2012 to read an EKM Omnimeter I v.3.

The library supports reading meter data including schedules and holidays.  The meter is referenced by a filedescriptor to a socket connected to the EKM iSerial or the serial port connected to the device.

The library currently supports setting the time, but not TOU schedules and holidays.

## library functions

#include <ekm.h>

### meter_open(int connection, struct meter_response * response, u_int64_t serial_number)
Open the meter and fill in the struct meter_response with parsed data from the meter.  Only one meter should be open on the RS485 bus at a time.  Call meter_close() at the end of the transaction or before opening another meter.

Takes an open connection to the meter serial port.  The connection can be a socket connected to a TCP-RS485 interface or an serial port. The serial_number selects which meter on the RS485 bus should be opened.

return values: 0 - Bad CRC, 1 - good CRC, any other value indicates a short read.

### meter_close(int connection)
End the transaction with open meters on the RS485 bus.  The serial port or socket remains open.

### meter_login(int connection, char * password)
Login to the meter to allow settings changes.

The default meter password is "00000000".

return values: 1 - success, 0 - failure

### readhistory(int connection, struct meter_history * history)
Read the meter 6 month Time Of Use history and fill in the struct meter_history.

return values: 1 - success, 0 - failure

### scheduleread(int connection, struct meter_schedule * schedule)
Read the Time Of Use and holidays schedules and fills in the struct meter_schedule.

return values: 1 - success, 0 - failure

### set_time(int connection)
Set the meter real time clock according to the local clock.

return values: 1 - success, 0 - failure

### ekm_flush(int connection)
Clear the connection input buffer.  There may be unread data in the iput buffer which will be read before the meter data.  This is an exposed private function and you shouldn't need to use it before calling the library functions. 
