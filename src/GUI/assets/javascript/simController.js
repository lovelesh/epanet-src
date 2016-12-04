//Javascript file with function implementations


$(function(){
   $('#start-button').prop("disabled", true);
   $('#cancel-button').prop("disabled", true);
   $('#advanced-button').prop("disabled", true);
   $('#path1').prop("disabled", true);
   $('#path2').prop("disabled", true);
   $('#path3').prop("disabled", true);
   $('#path4').prop("disabled", true);
   $('#path5').prop("disabled", true);
   $('#advancedOptions').hide();
   $('#download-button').prop("disabled", true);
   //$('#export-button').prop("disabled", true);
   $('#export-button').hide();
   
   $('#w1').prop("disabled", true);
   $('#w2').prop("disabled", true);
   $('#w3').prop("disabled", true);
   $('#w4').prop("disabled", true);
   $('#w5').prop("disabled", true);
   $('#w6').prop("disabled", true);
   
   $('#durationWarning').hide();
   $('#uploadSuccess').hide();
   $('#file-op-line').hide();
   
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


function showOpts() {
    $('#advancedOptions').show();
}

// Function to start simulation
function loadDoc() {
    
  $("#test").empty();  
  $('#start-button').prop("disabled", true);
  $('#cancel-button').prop("disabled", false);
  var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
  var message = {};
  if (Index == 0) {
      console.log("Type: " + Index + "\n");
      console.log("StartTime: " + $('#startTime').val() + "\n");
      console.log("Duration: " + $('#duration').val() + "\n");
    message = {
        Type: document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value,
        StartTime: $('#startTime').val(),
        Duration: $('#duration').val()
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
          $('#download-button').prop("disabled", false);
          $('#export-button').prop("disabled", false);
          $('#export-button').show();
      }
    });
  })
}

function changeType() {
    var Index = document.uploadForm.Type.options[document.uploadForm.Type.selectedIndex].value;
    if(Index == 0) {
        //$('#start-button').prop("disabled", true);
        //$('#cancel-button').prop("disabled", true);
        $('#advanced-button').prop("disabled", true);
        $('#path1').prop("disabled", false);
        $('#path2').prop("disabled", false);
        $('#path3').prop("disabled", false);
        $('#path4').prop("disabled", true);
        $('#path5').prop("disabled", true);
        $('#advancedOptions').prop("disabled", true);
        $('#advancedOptions').hide();
        $('#startTime').prop("disabled", false);
        $('#durationWarning').show();

        $('#w1').prop("disabled", true);
        $('#w2').prop("disabled", true);
        $('#w3').prop("disabled", true);
        $('#w4').prop("disabled", true);
        $('#w5').prop("disabled", true);
        $('#w6').prop("disabled", true);
        $('#startTime').val('0');

    }
    else if(Index == 1) {
        //$('#start-button').prop("disabled", true);
        //$('#cancel-button').prop("disabled", true);
        $('#advanced-button').prop("disabled", false);
        $('#path1').prop("disabled", false);
        $('#path2').prop("disabled", false);
        $('#path3').prop("disabled", false);
        $('#path4').prop("disabled", false);
        $('#path5').prop("disabled", false);
        $('#startTime').prop("disabled", true);
        $('#durationWarning').hide();
        
        $('#w1').prop("disabled", false);
        $('#w2').prop("disabled", false);
        $('#w3').prop("disabled", false);
        $('#w4').prop("disabled", false);
        $('#w5').prop("disabled", false);
        $('#w6').prop("disabled", false);
        $('#startTime').val('');
    }
    
}

// Function to halt an in-progress simulation

function cancelSimulation(){

$.ajax({
        url: '/cancelSimulation',
        async: 'true',
        type: "post",
        success: function(res) {
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


function viewFile() {

$.ajax({
        url: '/download',
        async: 'true',
        dataType: "text",
        type: "get",
        success: function(res) {
          var array = $.csv.toArrays(res);
          $('#file-op-line').show();
          $("#file-output").append(generateTable(array));
        },
        error: function(res) {
            $('#download-button').text('Failed');
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