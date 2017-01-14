var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var os = require('os');
var fs = require('fs');
var util  = require('util'),
    spawn = require('child_process').spawn;
var multer  = require('multer');

var client_binary;
var isSimulationStarted = 1;
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
   //Delete the intermediate files created by simulation process
    //  fs.readdir('./uploads', (err, files)=>{
  //       for (var i = 0, len = files.length; i < len; i++) {
//	    fs.unlink('./uploads/' + files[i]);
      //   }
     // });
   /*   fs.readdir('./uploads', (err, files)=>{
         for (var i = 0, len = files.length; i < len; i++) {
	    fs.unlink('./uploads/' + files[i]);
         }
      });
   */

   var uploadStatus = 0;
   upload(req,res,function(err) {
      filenames = req;
      if(err) {
	 uploadStatus = 1;
	 return res.end("Error uploading files.");
      }
   });
   if(uploadStatus) {
      res.status(500);   
   }
   console.log("SUCCESSFULLY UPLOADED");
   res.status(200);
   res.send('SUCCESS');
});


//============================================================
  
app.get('/download-tank', function(req, res) {
   res.download('../../result/tank.csv');
});

app.get('/download-valve', function(req, res) {
   res.download('../../result/valve.csv');
});

app.get('/download-job', function(req, res) {
   res.download('../../result/Job-Output.csv');
});

app.get('/download-sim-tank', function(req, res) {
    console.log("\nDownload request received for sim_tank.csv");
    res.download('../../result/sim_tank.csv');
});

app.get('/download-sim-valve', function(req, res) {
   console.log("\nDownload request received for sim_valve.csv");
   res.download('../../result/sim_valve.csv');
});

  
app.get('/', function(req, res) {
   /*var fsd = require('fs-extra')
      fsd.emptyDir('./uploads', function (err) {
	 if (!err) console.log('success!')
      })*/
   fs.readFile('index.html', 'utf8', function(err, text){
      console.log(req);
      res.send(text);
   });
});

app.post('/cancelSimulation', function(req, res) {
   var msg = {};
   if(client_binary){
      msg.Data = '------  Signal SIGHUP sent to running simulation ------';
      client_binary.kill('SIGHUP');
      client_binary = null;
   }
   else{
      msg.Data = 'Simulation already stopped/not running'; 
   }
   res.send(JSON.stringify(msg));
});  
  
http.listen(app.get('port'), function(){
   console.log("Express server listening on port " + app.get('port'));
})

io.on('connection', function(socket) {
    console.log('new user connected');

    socket.on('message', function(message) {
        //Delete the intermediate files created by simulation process
        fs.readdir('.', (err, files)=>{
            for (var i = 0, len = files.length; i < len; i++) {
                var match = files[i].match(/en.*/);
                if(match !== null)
                    fs.unlink(match[0]);
            }
        });
        //   fs.readdir('.', (err, files)=>{
        //      for (var i = 0, len = files.length; i < len; i++) {
        //	    var match = files[i].match(/en.*/);
        //	    if(match !== null)
        //	      fs.unlink(match[0]);
        //      }
        //   });

        obj = JSON.parse(message);
        Type = obj.Type;
        duration = obj.Duration;

        console.log(obj);

        if(Type == 0) {
            startTime = obj.StartTime;
            w12 = obj.w12;
            w3 = obj.w3;
            w4 = obj.w4;
            w5 = obj.w5;
            w6 = obj.w6;
            var file1 = './uploads/' + filenames.files[0].originalname;
            var file2 = './uploads/' + filenames.files[1].originalname;
            var file3 = './uploads/' + filenames.files[2].originalname;
            client_binary = spawn('../../build/Linux/epanet_wateropt' ,['0', file1, file2, file3, startTime, duration, w12, w3, w4, w5, w6]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                console.log('\nisSimulationStarted: ' + isSimulationStarted);
                socket.emit('newdata', JSON.stringify(msg));
            });
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
            client_binary = spawn('../../build/Linux/epanet_wateropt' ,['1', file1, file2, file3, file4, file5, duration, w12, w3, w4, w5, w6]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                socket.emit('newdata', JSON.stringify(msg));
            });
        }

        else if(Type == 2) {
            var file1 = './uploads/' + filenames.files[0].originalname;
            var file2 = './uploads/' + filenames.files[1].originalname;
            var file3 = './uploads/' + filenames.files[2].originalname;
            var file4 = './uploads/' + filenames.files[3].originalname;
            var file5 = './uploads/' + filenames.files[4].originalname;
            client_binary = spawn('../../build/Linux/epanet_wateropt' ,['2', file1, file2, file3, file4, file5]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                console.log('\nisSimulationStarted: ' + isSimulationStarted);
                socket.emit('newdata', JSON.stringify(msg));
            });
        }

        var msg = {};
        var chunk = '';

        /* Registering event listeners on first client binary execution */    

        /* Do some action whenever there is some data on standard
           output due to first binary execution */
        if(client_binary) {
            console.log('\nFLAG: ' + isSimulationStarted + '\n');
            if(isSimulationStarted) {
                client_binary.stdout.on('data', function(data){
                    msg.Data = chunk + data;
                    socket.emit('newdata', JSON.stringify(msg));
                });

                // Send a response to browser/client when first binary execution is completed

                client_binary.on('close', function (code) {
                    msg = {};
                    if(code == '0'){
                        msg.Result = 'All Iterations completed successfully, exit code : ' + code;
                    }
                    else{
                        msg.Result = 'Simulation ended abnormally, exit code : ' + code;
                    }
                    msg.ExitCode = code;
                    socket.emit('newdata', JSON.stringify(msg));
                    console.log('Process closed.');
                    client_binary = null;
                });


                client_binary.stderr.on('data', function (data) {
                    console.log('Failed to start child process 1.');
                });
            }
            else {
                msg = {};
                msg.Result = 'Error: Simulation cannot be started!';
                socket.emit('newdata', JSON.stringify(msg));
            }
        }
    });
})
