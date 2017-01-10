//Javascript file with function implementations


$(function(){

   //Register the form dynamic validation
   //$('#uploadForm').formValidation();
   $('#totalDuration').css('background-color', 'transparent');
   $('#start-button').prop("disabled", true);
   $('#cancel-button').prop("disabled", true);
   $('#advanced-button').prop("disabled", true);
   $('#path1').prop("disabled", true);
   $('#path2').prop("disabled", true);
   $('#path3').prop("disabled", true);
   $('#path4').prop("disabled", true);
   $('#path5').prop("disabled", true);
   $('#valveSolPath').prop("disabled", true);
   $('#advancedOptions').hide();
   $('#tank-view-button').prop("disabled", true);
   $('#valve-view-button').prop("disabled", true);
   $('#uploadButton').prop("disabled", true);

   $('#durationWarning').hide();
   $('#totalDurationRow').hide();
   $('#uploadSuccess').hide();
   $('#file-op-line').hide();
   $('#durationRow').hide();
   $('#startTimeRow').hide();
   $('#path2row').show();
   $('#path5row').hide();
   $('#joblist-file-row').hide();

   $('#startTime').on("input", setTotalDur);
   $('#duration').on("input", setTotalDur);

   var form = document.forms.namedItem("uploadForm");

   form.addEventListener('submit', function(ev) {

      var oData = new FormData(form);

      var oReq = new XMLHttpRequest();
      oReq.open("POST", "sumit-upload", true);
      oReq.onload = function(oEvent) {
	 if (oReq.status == 200) {
            $('#uploadSuccess').show();
	    $('#uploadStatus').html("Files Uploaded Successfully!");
	    $('#start-button').prop("disabled", false);
	 } 
	 else {
	    oOutput.innerHTML = "Error " + oReq.status + " occurred when trying to upload file.<br \/>";
	 }
      };

      oReq.send(oData);
      ev.preventDefault();
   }, false);

});

function setTotalDur() {
   var start = parseInt($('#startTime').val(),10),
       dur = parseInt($('#duration').val(),10);
   var sum = start+dur;
   if(isNaN(sum))
       sum = 0;
   if(sum <=24) {
       $('#totalDuration').css('background-color', '');
       $('#totalDuration').val(sum + " hours");
   }
   else {
      $('#totalDuration').css('background-color', 'bisque');
      $('#totalDuration').val(sum + " hours (Warning: Greater than 24)");
   }
}

function showOpts() {
    $('#advancedOptions').show();
}

// Function to start simulation
function loadDoc() {
    
  $("#test").empty();  
  $("#tank-file-output").empty();
  $("#valve-file-output").empty();
  $('#start-button').prop("disabled", true);
  $('#uploadSuccess').hide();
  $('#cancel-button').prop("disabled", false);
  $('#uploadButton').prop("disabled", true);
  $('#job-view-button').prop("disabled",true);
  $('#file-op-line').hide();
  var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
  var message = {};
  if (Index == 0) {
      console.log("Type: " + Index + "\n");
      console.log("StartTime: " + $('#startTime').val() + "\n");
      console.log("Duration: " + $('#duration').val() + "\n");
    message = {
        Type: '0',
        StartTime: $('#startTime').val(),
        Duration: $('#duration').val(),
        w12: $('#w12').val(),
        w3: $('#w3').val(),
        w4: $('#w4').val(),
        w5: $('#w5').val(),
        w6: $('#w6').val()
    };
  }
  else if(Index == 1) {
      message = {
        Type: '1',
        Duration: $('#duration').text(),
        w12: $('#w12').val(),
        w3: $('#w3').val(),
        w4: $('#w4').val(),
        w5: $('#w5').val(),
        w6: $('#w6').val()
    };
  }
  else if(Index == 2) {
      message = {
        Type: '2'
    };
  }
  
  var text='<thead><tr><th>Property</th><th>Value</th></tr></thead>';
  var socket = io();
  
  //Send all user entered data to nodejs server
  socket.send(JSON.stringify(message));
  var count = 0;
  socket.on('newdata', function(d){
    var json = JSON.parse(d);
    $.each(json, function(key, value){
        var table = $("#test");
        if(count < 13) {
          count++;
          table.append("<tr><td>" + count + "</td><td>" + value + "</td></tr>");
        }
        if(count >=13) {
          count=0;
          table.empty();
        }
      if(key == 'Result') {
         $('#start-button').prop("disabled", false);
         $('#cancel-button').prop("disabled", true);
      }  
      if(key == 'ExitCode' && value == 0) {
	  $('#tank-view-button').prop("disabled", false);
	  $('#valve-view-button').prop("disabled", false);
	  $('#export-button').prop("disabled", false);
	  $('#uploadButton').prop("disabled", false);
      }
    });
  })
}

function changeType() {
   $('#uploadSuccess').hide();
   var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
   if(Index == 0) {
      $('#uploadButton').prop("disabled", false);
      $('#advanced-button').prop("disabled", false);
      $('#path2row').show();
      $('#path5row').hide();
      $('#path1').prop("disabled", false);
      $('#path2').prop("disabled", false);
      $('#path3').prop("disabled", false);
      $('#path4').prop("disabled", true);
      $('#path5').prop("disabled", true);
      $('#valveSolPath').prop("disabled", true);
      $('#advancedOptions').hide();
      $('#startTimeRow').show();
      $('#startTime').prop("disabled", false);
      $('#durationWarning').hide();
      $('#totalDurationRow').show();
      $('#startTime').show();
      $('#durationRow').show();

      $('#startTime').val('0');
      $('#duration').prop("disabled", false);
   }
   else if(Index == 1) {
      $('#startTimeRow').hide();
      $('#advancedOptions').hide();
      $('#uploadButton').prop("disabled", false);
      $('#advanced-button').prop("disabled", false);
      $('#path2row').show();
      $('#path1').prop("disabled", false);
      $('#path2').prop("disabled", false);
      $('#path3').prop("disabled", false);
      $('#path4').prop("disabled", false);
      $('#path5').prop("disabled", false);
      $('#valveSolPath').prop("disabled", true);
      $('#startTime').prop("disabled", true);
      $('#durationWarning').show();
      $('#path5row').show();
      $('#totalDurationRow').hide();

      $('#durationRow').show();
      $('#startTimeRow').hide();
      $('#duration').prop("disabled", false);
   }
   else if(Index == 2) {
      $('#advancedOptions').hide();
      $('#uploadButton').prop("disabled", false);
      $('#advanced-button').prop("disabled", true);
      $('#valveSolPath').prop("disabled",false);
      $('#path2').prop("disabled", false);
      $('#path1').prop("disabled", false);
      $('#path3').prop("disabled", false);
      $('#path4').prop("disabled", false);
      $('#path5').prop("disabled", true);
      $('#startTime').prop("disabled", true);
      $('#durationWarning').hide();
      $('#totalDurationRow').hide();
      $('#path2row').show();
      $('#path5row').hide();
      $('#startTime').prop("disabled", true);
      $('#duration').prop("disabled", true);

      $('#durationRow').hide();
      $('#startTimeRow').hide();
   }
}

// Function to halt an in-progress simulation

function cancelSimulation(){

   $.ajax({
      url: '/cancelSimulation',
      async: 'true',
      type: "post",
      success: function(res) {
	 $('#uploadButton').prop("disabled", false);
	 $('#cancel-button').prop("disabled", true);
	 $('#start-button').prop("disabled", false);
	 var json = JSON.parse(res);
	 $.each(json, function(key, value){
	    var table = $("#test");
	    table.append("<tr><td></td><td>" + value + "</td></tr>"); 
	 });
      },
      error: function(res) {
	 $('#start-button').text('Failed');
	 $('#testArea').html('Failure response:' + res);
      }
   });
}


function viewTankFile() {

   var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
   var downloadUrl;
   if(Index == '2'){
      downloadUrl= '/download-sim-tank';
      $('#sim-tank-download').show();
      $('#sim-valve-download').show();
      $('#valve-download').hide();
      $('#tank-download').hide();
   }
   else{
      viewJoblistFile();
      downloadUrl= '/download-tank';
      $('#sim-tank-download').hide();
      $('#sim-valve-download').hide();
      $('#valve-download').show();
      $('#tank-download').show();
   }
   $("#tank-file-output").empty();

   $('#tank-view-button').prop("disabled", true);
   $.ajax({
      url: downloadUrl,
      async: 'true',
      dataType: 'text',
      type: "get",
      success: function(res) {
	 viewValveFile();
	 var array = $.csv.toArrays(res);
	 $('#file-op-line').show();
	 $('#tank-op-line').show();
	 $("#tank-file-output").append(generateTable(array));
      },
      error: function(res) {
	 $('#download-button').text('Failed');
	 $('#testArea').html('Failure response:' + res);
      }
   });
}

function viewValveFile() {

   var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
   var downloadUrl;
   if(Index == '2'){
      downloadUrl= '/download-sim-valve';
   }
   else{
      downloadUrl= '/download-valve';
   }

   $("#valve-file-output").empty();
   $.ajax({
      url: downloadUrl,
      async: 'true',
      dataType: "text",
      type: "get",
      success: function(res) {
	 var array = $.csv.toArrays(res);
	 $('#file-op-line').show();
	 $('#valve-op-line').show();
	 $("#valve-file-output").append(generateTable(array));
	 plotGraphFromCsv(Index);
      },
      error: function(res) {
	 $('#download-button').text('Failed');
	 $('#testArea').html('Failure response:' + res);
      }
   });
}


function viewJoblistFile() {

   var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
   var downloadUrl= '/download-job';
   $("#job-file-output").empty();
   $.ajax({
      url: downloadUrl,
      async: 'true',
      dataType: "text",
      type: "get",
      success: function(res) {
	 var array = $.csv.toArrays(res);
         $('#joblist-file-row').show();
	 $("#job-file-output").append(generateTable(array));
	 plotGraphFromCsv(Index);
      },
      error: function(res) {
	 $('#download-button').text('Downloading Failed');
	 $('#testArea').html('Failure response:' + res);
      }
   });
}

function generateTable(data) {
    var html = '';

    if(typeof(data[0]) === 'undefined') {
        return null;
    }

    if(data[0].constructor === String) {
        html += '<tr>\r\n';
        for(var item in data) {
            html += '<td>' + data[item] + '</td>\r\n';
        }
        html += '</tr>\r\n';
    }

    if(data[0].constructor === Array) {
        for(var row in data) {
            html += '<tr>\r\n';
            for(var item in data[row]) {
            html += '<td>' + data[row][item] + '</td>\r\n';
            }
            html += '</tr>\r\n';
        }
    }

    if(data[0].constructor === Object) {
        for(var row in data) {
            html += '<tr>\r\n';
            for(var item in data[row]) {
            html += '<td>' + item + ':' + data[row][item] + '</td>\r\n';
            }
            html += '</tr>\r\n';
        }
    }
    
    return html;
}
