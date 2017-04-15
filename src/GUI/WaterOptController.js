var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var os = require('os');
var fs = require('fs-extra');
var util  = require('util'),
    spawn = require('child_process').spawn;
var multer  = require('multer');
var Hashmap = require('hashmap');
var qs = require('querystring');

app.use(express.static('assets'));
app.set('port', process.env.PORT || 7000);
//var client_binary;
var isSimulationStarted = 1;
var filenames;
var obj,Type,startTime,duration,w12,w3,w4,w5,w6;
var userCount = 0;
var binary_map = new Hashmap();

//=======================================================

var storage = multer.diskStorage({
    destination: function (req, file, callback) {
        callback(null, './uploads/');
    },
    filename: function (req, file, callback) {
        callback(null, file.originalname);
    }
});

var upload = multer({ storage : storage}).any();

//app.post('/sumit-upload',function(req,res){
app.post('/p',function(req,res){
	var uid = req.query.uid;
	var uploadStatus = 0;
	upload(req,res,function(err) {
		console.log("*************  body: %j", req.body);
		filenames = req;
		if(err) {
			uploadStatus = 1;
			return res.end("Error uploading files.");
		}
		else {
			if(!fs.existsSync('./uploads/'+uid)){
				fs.mkdirSync('./uploads/'+uid);
			}
			req.files.forEach(function(f) {
				//console.log(f);
				fs.rename(f.path, './uploads/' + uid + '/' + f.originalname, function (err) {
					if (err) throw err;
					console.log('renamed complete');
				});
			});
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

app.post('/removeSession',function(req,res){
    var uid = req.query.uid;

    console.log("request for cleanup for uid " + uid + " received");
    if(binary_map.has(uid)) {
        binary_map.remove(uid);
    }

    var path = './uploads/' + uid;

    if(fs.existsSync(path)){
        fs.removeSync(path);
    }
    console.log("cleanup for uid " + uid + " , path " + path + " complete!");
    res.status(200);
    res.send('SUCCESS');
});
  
app.get('/download-tank', function(req, res) {
	var uid = req.query.uid;
    res.download('../../result/' + uid +'/tank.csv');
});

app.get('/download-valve', function(req, res) {
	var uid = req.query.uid;
    res.download('../../result/' + uid +'/valve.csv');
});

app.get('/download-job', function(req, res) {
	var uid = req.query.uid;
    res.download('../../result/' + uid + '/Job-Output.csv');
});

app.get('/download-sim-tank', function(req, res) {
	var uid = req.query.uid;
    console.log("\nDownload request received for sim_tank.csv");
    res.download('../../result/' + uid + '/sim_tank.csv');
});

app.get('/download-sim-valve', function(req, res) {
	var uid = req.query.uid;
    console.log("\nDownload request received for sim_valve.csv");
    res.download('../../result/' + uid + '/sim_valve.csv');
});

  
app.get('/', function(req, res) {
    fs.readFile('index.html', 'utf8', function(err, text){
        console.log(req);
        res.send(text);
    });
});

app.post('/cancelSimulation', function(req, res) {
    var msg = {};
    var body='';
    var id;
    req.on('data', function(data){
        body += data;   
        console.log("***** BODY IS: " + body);
        var post = qs.parse(body);
        id = post['uid'];
    });
    req.on('end', function(){
        var post = qs.parse(body);
        id = post['uid'];
        var bin = binary_map.get(id);

        if(bin){
            msg.Data = '------ Signal SIGHUP sent to running simulation ------';
            bin.kill('SIGHUP');
            bin = null;
            binary_map.remove(id);
        }
        else {
            msg.Data = 'Simulation already stopped/not running'; 
        }
        res.send(JSON.stringify(msg));
    });
});  

http.listen(app.get('port'), function(){
    console.log("Express server listening on port " + app.get('port'));
});

io.on('connection', function(socket) {
    userCount++;
    console.log('' + userCount + ' users connected');
    socket.on('message', function(message) {
        //Delete the intermediate files created by simulation process
        fs.readdir('.', (err, files)=>{
            for (var i = 0, len = files.length; i < len; i++) {
                var match = files[i].match(/en.*/);
                if(match !== null)
                    fs.unlink(match[0]);
            }
        });
        obj = JSON.parse(message);
        Type = obj.Type;
        duration = obj.Duration;
        var uid = obj.uid;

        console.log(obj);

        if(Type == 0) {
            startTime = obj.StartTime;
            w12 = obj.w12;
            w3 = obj.w3;
            w4 = obj.w4;
            w5 = obj.w5;
            w6 = obj.w6;
            var fpath = './uploads/' + uid;
            var file1 = fpath + '/' + filenames.files[0].originalname;
            var file2 = fpath + '/' + filenames.files[1].originalname;
            var file3 = fpath + '/' + filenames.files[2].originalname;
            var client_binary = spawn('../../build/Linux/epanet_wateropt' ,['0', file1, file2, file3, startTime, duration, w12, w3, w4, w5, w6]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                console.log('\nisSimulationStarted: ' + isSimulationStarted);
                socket.emit('newdata', JSON.stringify(msg));
            });
            binary_map.set(obj.uid,client_binary);
        }

        else if(Type == 1) {
            w12 = obj.w12;
            w3 = obj.w3;
            w4 = obj.w4;
            w5 = obj.w5;
            w6 = obj.w6;
            var fpath = './uploads/' + uid;
            var file1 = fpath + '/' + filenames.files[0].originalname;
            var file2 = fpath + '/' + filenames.files[1].originalname;
            var file3 = fpath + '/' + filenames.files[2].originalname;
            var file4 = fpath + '/' + filenames.files[3].originalname;
            var file5 = fpath + '/' + filenames.files[4].originalname;
            var client_binary = spawn('../../build/Linux/epanet_wateropt' ,['1', file1, file2, file3, file4, file5, duration, w12, w3, w4, w5, w6]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                socket.emit('newdata', JSON.stringify(msg));
            });
            binary_map.set(obj.uid,client_binary);
            console.log("***************** CL_BIN :" + client_binary);
        }

        else if(Type == 2) {
            var fpath = './uploads/' + uid;
            var file1 = fpath + '/' + filenames.files[0].originalname;
            var file2 = fpath + '/' + filenames.files[1].originalname;
            var file3 = fpath + '/' + filenames.files[2].originalname;
            var file4 = fpath + '/' + filenames.files[3].originalname;
            var file5 = fpath + '/' + filenames.files[4].originalname;
            var client_binary = spawn('../../build/Linux/epanet_wateropt' ,['2', file1, file2, file3, file4, file5]);
            client_binary.on('error', (err) => {
                isSimulationStarted = 0;
                var msg={};
                msg.Result = "Unable to spawn binary at path" + err.path;
                console.log('whoops! There was an uncaught error', JSON.stringify(msg));
                console.log('\nisSimulationStarted: ' + isSimulationStarted);
                socket.emit('newdata', JSON.stringify(msg));
            });
            binary_map.set(obj.uid,client_binary);
        }

        var msg = {};
        var chunk = '';

        /* Registering event listeners on first client binary execution */    

        /* Do some action whenever there is some data on standard
           output due to first binary execution */
        if(binary_map.get(uid)) {
            console.log('\nFLAG: ' + isSimulationStarted + '\n');
            if(isSimulationStarted) {
                binary_map.get(uid).stdout.on('data', function(data){
                    msg.Data = chunk + data;
                    socket.emit('newdata', JSON.stringify(msg));
                });

                // Send a response to browser/client when first binary execution is completed

                binary_map.get(uid).on('close', function (code) {
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

                binary_map.get(uid).stderr.on('data', function (data) {
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
	socket.on('disconnect', function() {
        console.log("***************** CLIENT DISCONNECTED *****************");
	});
});
