## Central ghome server :
This server receives data from the sensors/actuators proxy, updates the informations, and warn the web-server about the modifications.
On the other side it receves user-defined rules, from the web server, about the order to give to the actuators upon some sensors-events.

Each time a new value, or a new rule, is received the rules are checked, and if some of them are activated by the new values, corresponding orders are sent to the actuators.

The port used for tcp connection with the sensors and the web-server can be changed in the ghome.conf config file. Logs can be accessed in ghome.log.
