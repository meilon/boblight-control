Pebble.addEventListener("ready",
  function (e) {
    console.log("JavaScript app ready and running!");
    var url = 'rpi-due.fritz.box:8080/socket.io/1/';
    var sid;
    var heartbeattimeout;
    var connclosetimeout;
    var transports;
    var status;
    var presets;

    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", 'http://' + url, true);
    xmlhttp.onload = function () {
      var elements = this.responseText.split(':');
      sid = elements[0];
      heartbeattimeout = elements[1];
      connclosetimeout = elements[2];
      transports = elements[3].split(',');
      console.log(this.responseText);

      var sock = new WebSocket('ws://' + url + 'websocket/' + sid);

      Pebble.addEventListener("appmessage",
        function (e) {
          console.log("Received message: " + JSON.stringify(e));
          if(e.payload.init !== undefined) {
            console.log("Requested init package");
            sock.send('5:::{"name":"status","args":["' + status + '"]}');
            Pebble.sendAppMessage(presets);
          }
          if(e.payload.status !== undefined) {
            console.log("Updating status to: " + e.payload.status);
            sock.send('5:::{"name":"status","args":["' + (e.payload.status == 1 ? 'on' : 'off') + '"]}');
          }
          if(e.payload.red !== undefined) {
            console.log("Updating red to:    " + e.payload.red);
            sock.send('5:::{"name":"red","args":["' + e.payload.red + '"]}');
          }
          if(e.payload.green !== undefined) {
            console.log("Updating green to:  " + e.payload.green);
            sock.send('5:::{"name":"green","args":["' + e.payload.green + '"]}');
          }
          if(e.payload.blue !== undefined) {
            console.log("Updating blue to:   " + e.payload.blue);
            sock.send('5:::{"name":"blue","args":["' + e.payload.blue + '"]}');
          }
          if(e.payload.preset !== undefined) {
            console.log("Updating preset to: " + e.payload.preset);
            sock.send('5:::{"name":"preset","args":["' + e.payload.preset + '"]}');
          }
        }
      );

      sock.onopen = function () {
        console.log("Socket has been opened!");
      };
      sock.onclose = function () {
        console.log("Socket closed");
      };
      sock.onmessage = function (e) {
        console.log('Server: ' + e.data);
        var msg = {};

        switch (e.data.substr(0, 1)) {
          //Connect
          case '1':
            //nothing to do
            break;
          //Heartbeat
          case '2':
            console.log('Heartbeat request, replying');
            sock.send('2::');
            break;
          //Event
          case '5':
            var event = JSON.parse(e.data.substr(e.data.indexOf(':{') + 1));
            var command = event["name"];
            var args = event['args'];
            console.log('Event with name: ' + command);
            
            
            switch (command) {
              case 'currData':
                status = args[0]['status'];
                msg.status = (args[0]['status'] == 'on' ? 1 : 0);
                msg.red = parseInt(args[0]['red'], 10);
                msg.green = parseInt(args[0]['green'], 10);
                msg.blue = parseInt(args[0]['blue'], 10);
                msg.preset =  parseInt(args[0]['preset'], 10);
                console.log(JSON.stringify(msg));
                break;
              case 'presets':
                msg.presets = "Rot:Grün:Warmweiß hell:Warmweiß dunkel";
                presets = msg;
                break;
            }
            Pebble.sendAppMessage(msg);
            break;
        }
      };
    };
    xmlhttp.send(null);
  }
);