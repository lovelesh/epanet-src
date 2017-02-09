function plotGraphFromCsv(mode) {

	function GETCSVFILE (url) {
		var jqXHR = $.ajax({
			async: false,
			type: "GET",
			url: url,
			dataType: "csv",
		});
		$('.highcharts-title').remove();
		$('.highcharts-credits').html('');
		return jqXHR.responseText;
	}
	//Getting the files from the file specified
	var i, j = -1, flag = 0, len;
	var TankFirstAndlastRow = [];
	var ValveFirstAndlastRow = [];
	var csv, json;

	(function () {
		var downloadUrl;
		if(mode=='2'){
			console.log("Mode=2, download sim tank");
			csv = GETCSVFILE ('/download-sim-tank');
		}
		else{
			console.log("Mode=" + mode + ", download sim tank");
			csv = GETCSVFILE ('/download-tank');
		}

		json = CSV2JSON(csv, TankFirstAndlastRow);
		$('#container1').highcharts({
			plotOptions: {
				line: {
					dataLabels: {
						enabled: false
					},
					enableMouseTracking: true,
					marker: {
						enabled: true,
						fillColor: '#FFFFFF',
						lineWidth: 2,
						lineColor: null,
						radius: 3
					},
					scrollbar: {
						enabled: true
					},
					allowPointSelect: true
				}
			},
			chart: {
				type: 'line',
				zoomType: 'xAxis',
				style: {
					fontFamily: 'segoe'
				}
			},
			xAxis: {
				title: {
					text: 'Time (hours)'
				},
				categories: [],
				type: 'category',
				allowDecimals: false                       	
			},
			yAxis: {
				title: {
					text: 'Water Level (%)'
				},
				plotLines: [{
					value: 0,
					width: 1,
					color: '#808080'
				}]
			},
			series: [{}]
		});	
		$('.highcharts-title').remove();
		$('.highcharts-credits').html('');
		var chart1 = $('#container1').highcharts();

		$.each(json, function(key, value){
			i = 0, ++j;
			chart1.xAxis[0].categories.push(json[j].Time);
			$.each(value, function(key, value){
				if (key != 'Time' && key != 'time' && key != '' && value != '') {
					if (flag == 0) {
						//addSeries
						chart1.addSeries({                        
							name: key,
							data: (function () {
								var data1 = [];
								data1.push([
										json[j].Time,
										parseFloat(value)
								]);
								return data1;
							}()),
							allowPointSelect: true,
							pointStart: json[j].Time,
						}, false, false);
					} else {
						if (flag == 1) {
							chart1.series[0].remove();
							flag = 2;
						}
						//addPoint
						chart1.series[i++].addPoint([json[j].Time, parseFloat(value)], true, false, false);
					}
				}
			});
			if (flag == 0)
				flag = 1;
		});
		/*
		   for downloading the coverted json file
		   window.open("data:text/json;charset=utf-8," + escape(json))
		   */
		$('#container1_header').removeClass('gifLoading');
		$('#container1_header').css({
			'padding-bottom' : '0px'
		});
	}) ();


	(function () {
		var downloadUrl;
		if(mode=='2'){
			csv = GETCSVFILE ('/download-sim-valve');  
		}
		else{
			csv = GETCSVFILE ('/download-valve');
		}       

		json = CSV2JSON(csv, ValveFirstAndlastRow);
		$('#container2').highcharts({
			plotOptions: {
				line: {
					dataLabels: {
						enabled: false
					},
					enableMouseTracking: true,
					marker: {
						enabled: true,
						fillColor: '#FFFFFF',
						lineWidth: 2,
						lineColor: null,
						radius: 3
					},
					scrollbar: {
						enabled: true
					},
					allowPointSelect: true
				}
			},
			chart: {
				type: 'line',
				zoomType: 'xAxis',
				style: {
					fontFamily: 'segoe'
				}
			},
			xAxis: {
				title: {
					text: 'Time (hours)'
				},
				type: 'category',
				allowDecimals: false,
				categories: []
			},
			yAxis: {
				title: {
					text: 'Flow (lps)'
				},
				plotLines: [{
					value: 0,
					width: 1,
					color: '#808080'
				}]
			},
			series: [{}]
		});
		$('.highcharts-title').remove();
		$('.highcharts-credits').html('');
		var chart2 = $('#container2').highcharts();
		//re-intializing the variables required.
		j = -1, flag = 0;

		$.each(json, function(key, value){
			i = 0, ++j;
			chart2.xAxis[0].categories.push(json[j].Time);
			$.each(value, function(key, value){
				if (key != 'Time' && key != 'time' && key != '' && value != '') {
					if (flag == 0) {
						//addSeries
						chart2.addSeries({                        
							name: key,
							data: (function () {
								var data2 = [];
								data2.push([
										json[j].Time,
										parseFloat(value)
								]);
								return data2;
							}()),
							allowPointSelect: true,
							pointStart: json[j].Time,
						}, false, false);
					} else {
						if (flag == 1) {
							chart2.series[0].remove();
							flag = 2;
						}
						//addPoint
						chart2.series[i++].addPoint([json[j].Time, parseFloat(value)], true, false, false);
					}
				}
			});
			if (flag == 0)
				flag = 1;
		});
		/*
		   for downloading the coverted json file
		   window.open("data:text/json;charset=utf-8," + escape(json))
		   */
		$('#container2_header').removeClass('gifLoading');
		$('#container2_header').css({
			'padding-bottom' : '0px'
		});
	}) ();

	/* the Line graphs Tank */
	(function () {
		$('#container3').highcharts({
			plotOptions: {
				column: {
					pointPadding: 1,
					borderWidth: 0,
					groupPadding: 2,
					shadow: false,
				}
			},
			chart: {
				type: 'column',
				zoomType: 'xAxis'
			},
			xAxis: {
				categories: []                     	
			},
			yAxis: {
				labels: {
					formatter: function() {
						return this.value + ' %';
					}
				},
				gridLineWidth: 1,
				tickInterval: 5
			},
			credits: {
				enabled: false
			},
			series: [{}]
		});	
		var chart3 = $('#container3').highcharts();
		//re-intializing the variables required.
		j = -1, flag = 0;
		$.each(TankFirstAndlastRow, function(key, value){
			i = 1;
			chart3.xAxis[0].categories.push(key);
			$.each(value, function(key, value){
				chart3.xAxis[0].categories.push(key);
				//addSeries
				chart3.addSeries({   
					name: " ",                     
					data: (function () {
						var data3 = [];
						data3.push([
								i,
								parseFloat(value)
						]);
						return data3;
					}()),
					allowPointSelect: true,
					pointStart: i,
				}, true, true);
				chart3.series[0].name = '';
				i++;
			});
		});
		$('#container3_header').removeClass('gifLoading');
		$('#container3_header').css({
			'padding-bottom' : '0px'
		});
	}) ();



	/* the Line graphs Valve */
	(function () {
		$('#container4').highcharts({
			plotOptions: {
				column: {
					pointPadding: 1,
					borderWidth: 0,
					groupPadding: 2,
					shadow: false,
				}
			},
			chart: {
				type: 'column',
				zoomType: 'xAxis'
			},
			xAxis: {
				categories: []                     	
			},
			yAxis: {
				labels: {
					formatter: function() {
						return this.value + ' %';
					}
				},
				gridLineWidth: 1,
				tickInterval: 5
			},
			credits: {
				enabled: false
			},
			series: [{}]
		});	
		var chart4 = $('#container4').highcharts();
		//re-intializing the variables required.
		j = -1, flag = 0;
		$.each(ValveFirstAndlastRow, function(key, value){
			i = 1;
			chart4.xAxis[0].categories.push(key);
			$.each(value, function(key, value){
				chart4.xAxis[0].categories.push(key);
				//addSeries
				chart4.addSeries({   
					name: " ",                     
					data: (function () {
						var data4 = [];
						data4.push([
								i,
								parseFloat(value)
						]);
						return data4;
					}()),
					allowPointSelect: true,
					pointStart: i,
				}, true, true);
				chart4.series[0].name = '';
				i++;
			});
		});
		$('#container4_header').removeClass('gifLoading');
		$('.highcharts-title').html('');
		$('#container4_header').css({
			'padding-bottom' : '0px'
		});
	}) ();

	//End of getting the files from the file specified
	function CSVToArray(strData, strDelimiter) {
		// Check to see if the delimiter is defined. If not,
		// then default to comma.
		strDelimiter = (strDelimiter || ",");
		// Create a regular expression to parse the CSV values.
		var objPattern = new RegExp((
					// Delimiters.
					"(\\" + strDelimiter + "|\\r?\\n|\\r|^)" +
					// Quoted fields.
					"(?:\"([^\"]*(?:\"\"[^\"]*)*)\"|" +
					// Standard fields.
					"([^\"\\" + strDelimiter + "\\r\\n]*))"), "gi");
		// Create an array to hold our data. Give the array
		// a default empty first row.
		var arrData = [[]];
		// Create an array to hold our individual pattern
		// matching groups.
		var arrMatches = null;
		// Keep looping over the regular expression matches
		// until we can no longer find a match.
		while (arrMatches = objPattern.exec(strData)) {
			// Get the delimiter that was found.
			var strMatchedDelimiter = arrMatches[1];
			// Check to see if the given delimiter has a length
			// (is not the start of string) and if it matches
			// field delimiter. If id does not, then we know
			// that this delimiter is a row delimiter.
			if (strMatchedDelimiter.length && (strMatchedDelimiter != strDelimiter)) {
				// Since we have reached a new row of data,
				// add an empty row to our data array.
				arrData.push([]);
			}
			// Now that we have our delimiter out of the way,
			// let's check to see which kind of value we
			// captured (quoted or unquoted).
			if (arrMatches[2]) {
				// We found a quoted value. When we capture
				// this value, unescape any double quotes.
				var strMatchedValue = arrMatches[2].replace(
						new RegExp("\"\"", "g"), "\"");
			} else {
				// We found a non-quoted value.
				var strMatchedValue = arrMatches[3];
			}
			// Now that we have our value string, let's add
			// it to the data array.
			arrData[arrData.length - 1].push(strMatchedValue);
		}
		// Return the parsed data.
		return (arrData);
	}
	function CSV2JSON(csv, firstAndlastRow) {
		var array = CSVToArray(csv);
		var objArray = [];

		for (i = 1; i < array.length - 1; i++) {
			objArray[i - 1] = {};
			var inFlag = 0;

			firstAndlastRow[0] = {};
			for (k = 0; k < array[0].length && k < array[i].length; k++) {
				var key = array[0][k];
				if (key != "")
					objArray[i - 1][key] = array[i][k];
				if (i == array.length - 2 && objArray[0][key] != "" && objArray[0][key] != undefined){
					if (inFlag == 1){
						if (objArray[i-1][key] != 0 && objArray[0][key] != 0)
							firstAndlastRow[0][key] = ((objArray[i-1][key] - objArray[0][key]) / objArray[0][key]) * 100;
					}
					inFlag = 1;
				}
			}
		}
		return objArray;
	}

}
