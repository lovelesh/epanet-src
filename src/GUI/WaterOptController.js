var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var os = require('os');
var fs = require('fs');
var util  = require('util'),
    spawn = require('child_process').spawn;
var multer  = require('multer');

//var upload = multer({ dest: 'uploads/' });


var client_binary;
var filenames;
var obj,Type,startTime,duration,w12,w3,w4,w5,w6;
  app.use(express.static('assets'));
  app.set('port', process.env.PORT || 7000);
  
  
  //=======================================================

var storage =   multer.diskStorage({
  destination: function (req, file, callback) {
    callback(null, './uploads/');
  },
  filename: function (req, file, callback) {
    callback(null, file.originalname);
  }
});

var upload = multer({ storage : storage}).any();

app.post('/sumit-upload',function(req,res){
    var uploadStatus = 0;
    upload(req,res,function(err) {
        filenames = req;
        if(err) {
            uploadStatus = 1;
            return res.end("Error uploading files.");
        }
        if(uploadStatus) {
            res.status(500);
        }
        else {
            res.status(200);
        }
    });
    console.log("SUCCESSFULLY UPLOADED");
    res.status(200);
    res.send('SUCCESS');
});


  //============================================================
  
  
  app.get('/', function(req, res) {
     fs.readFile('index.html', 'utf8', function(err, text){
	console.log(req);
	res.send(text);
     });
  });

  app.post('/cancelSimulation', function(req, res) {
    var msg = {};
    if(client_binary){
      msg.Data = 'Sending signal SIGHUP to the running client.';
      client_binary.kill('SIGHUP');
      client_binary = null;
    }
    else{
        msg.Data = 'Client1 already stopped/not running'; 
    }
    res.send(JSON.stringify(msg));
  });  
  
http.listen(app.get('port'), function(){
  console.log("Express server listening on port " + app.get('port'));
})

io.on('connection', function(socket) {
    console.log('new user connected');
    
    socket.on('message', function(message) {
        obj = JSON.parse(message);
        Type = obj.Type;
        duration = obj.Duration;
        console.log("\nSUMIT:  " + "Type: " + Type + "\n\n");
        console.log("RECEIVED : " + JSON.stringify(message, null, 2));
    
    
    if(Type == 0) {
      startTime = obj.StartTime;
      var file1 = './uploads/' + filenames.files[0].originalname;
      var file2 = './uploads/' + filenames.files[1].originalname;
      var file3 = './uploads/' + filenames.files[2].originalname;
      //client_binary = spawn('/home/amit/AMIT/epanet/waterProject/epanet-src/build/Linux_WISL09_temp/epanet_valve_optim_wisl09' ,['0', './uploads/Devanoor_10DMA_demand_zero_changed.inp', './uploads/temp_demand_all.csv', './uploads/Input_Files/joblist.txt', startTime, duration]);
      client_binary = spawn('../../build/Linux_WISL09_temp/epanet_valve_optim_wisl09' ,['0', file1, file2, file3, startTime, duration]);
    }
    
    else if(Type == 1) {
        w12 = obj.w12;
        w3 = obj.w3;
        w4 = obj.w4;
        w5 = obj.w5;
        w6 = obj.w6;
      var file1 = './uploads/' + filenames.files[0].originalname;
      var file2 = './uploads/' + filenames.files[1].originalname;
      var file3 = './uploads/' + filenames.files[2].originalname;
      var file4 = './uploads/' + filenames.files[3].originalname;
      var file5 = './uploads/' + filenames.files[4].originalname;
   	console.log(file1); 
   	console.log(file2); 
   	console.log(file3); 
   	console.log(file4); 
   	console.log(file5); 
	client_binary = spawn('../../build/Linux_WISL09_temp/epanet_valve_optim_wisl09' ,['1', file1, file2, file3, file4, file5, duration, w12, w3, w4, w5, w6]);
    }

  //  client_binary = spawn('/home/sumit/Desktop/sumit/WaterOpt/a.sh');
    
    console.log("startTime: " + startTime + "\nDuration: " + duration + "\nw12: " + w12 + "\nw3: " + w3 + "\nw4: " + w4 + "\nw5: " + w5 + "\nw6: " + w6 + "\n");
    
    var msg = {};
    var chunk = '';
    
    
    /* Registering event listeners on first client binary execution */    
    
    /* Do some action whenever there is some data on standard
       output due to first binary execution */
    client_binary.stdout.on('data', function(data){
        msg.Data = chunk + data;
        socket.emit('newdata', JSON.stringify(msg));
    });
    
    // Send a response to browser/client when first binary execution is completed
     
    client_binary.on('close', function (code) {
        msg = {};
        msg.Result = 'All Iterations completed, process closed with code ' + code;
        socket.emit('newdata', JSON.stringify(msg));
        console.log('Process closed.');
        client_binary = null;
    });

    
    client_binary.stderr.on('data', function (data) {
        console.log('Failed to start child process 1.');
    })
  });
})
