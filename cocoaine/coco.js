var s;
var i = 0;
var c = [];
var x = 100;

var move = function(dx,dy) {
	this.attr({
		transform: this.data('origTransform') + (this.data('origTransform') ? "T" : "t") + [dx, dy]
	});
}

var start = function() {
        this.data('origTransform', this.transform().local );
	this.attr({ fill: "red"});
}

var stop = function() {
	this.attr({ fill: "cyan"});
}

function addComponent()
{
	x += 110;
	c[i] = s.rect(x, 100, 100, 50);
	c[i].attr({
		fill: "cyan",
		stroke: "#000",
		strokeWidth: 5
	});
	s.append(c[i]);
	c[i].drag(move, start, stop);
	i++;
}

function notimplemented()
{
	alert("Not implemented (yet)");
}

var json = null;
var init = true;
var plots_init = true;

var t = [];
t.push([]);
t.push([]);
t.push([]);
t.push([]);

function plots(stats)
{
	var plotList = [];
	var taskNames = [];
	var mu = [];
	var mua = [];
	var mub = [];
	var sigma = [];
	var values = [];
	for (var i=0; i<stats.length; i++)
	{
		taskNames.push(stats[i].name);
		mu.push(stats[i].time_exec_mean);
		mua.push(stats[i].time_exec_mean - stats[i].time_exec_stddev);
		mub.push(stats[i].time_exec_mean + stats[i].time_exec_stddev);
		sigma.push(stats[i].time_exec_stddev);
	}
//	values.push({x: taskNames, y: mua, type: "bar", name: "Mean - StdDev"});
	values.push({x: taskNames, y: mu, type: "bar", name: "Mean"});
//	values.push({x: taskNames, y: mub, type: "bar", name: "Mean + StdDev"});
	values.push({x: taskNames, y: sigma, type: "bar", name: "StdDev"});
	plotList.push({
		handler: $("#taskStats")[0],
		values: values,
		layout: {
			title: "Tasks",
			width: 700,
			height: 350,
			margin: { t: 0 },
			xaxis: {
				title: 'Task'
			},
			yaxis: {
				title: 'Execution Time (s)'
			}
//			,barmode: "stack"
		},
		options: {
			showLink: false
		}
	});

	values = [];
	for (var i=0; i<stats.length; i++)
	{
		t[i].push(stats[i].time);
		values.push({y: t[i], mode: "lines", name: taskNames[i]});
	}

	plotList.push({
		handler: $("#taskTimes")[0],
		values: values,
		layout: {
			title: "Time",
			width: 700,
			height: 350,
			margin: { t: 0 },
			xaxis: {
				title: 'Task'
			},
			yaxis: {
				title: 'Execution Time (s)'
			}
		},
		options: {
			showLink: false
		}
	});

	for (var i=0; i < plotList.length; i++)
	{
		var p = plotList[i];
		if (plots_init)
			Plotly.newPlot(p.handler, p.values, p.layout, p.options);
		else
			Plotly.redraw(p.handler);
	}
	plots_init = false;
	/*
	var graph = $("#taskTimes")[0];
	var tasks = [];
	var values = [];
	var values_mean = [];
	var values_min = [];
	var values_max = [];
	for (i=0; i < graphs.length; i++)
	{
		var g = graphs[i];
		parts = g.name.split("_");
		if (parts.length == 1)
			continue;
		if (parts[0] == "t")
		{
			tasks.push(parts[1]);
			values.push({name: parts[1], y: g.data});
		}else if (parts[0] == "tm")
			values_mean.push(g.data[0]);
		else if (parts[0] == "tv")
		{
			values_min.push(values_mean[i] - 1);///Math.sqrt(g.data[0]));
			values_max.push(values_mean[i] + 1);//Math.sqrt(g.data[0]));
		}
	}	
	if (plots_init)
	{
		Plotly.newPlot(graph, values,
			{
				title: "Computation Time of Tasks",
				width: 700,
				height: 350,
				margin: { t: 0 }
			},
			{
				showLink: false
			}
		);
	}else
	{
		graph.data = values;
		Plotly.redraw(graph);
	}
	
	graph = $("#taskStats")[0];
	values = [];
	values.push({type: 'bar', name: 'min', x: tasks, y: values_min});
	values.push({type: 'bar', name: 'mean', x: tasks, y: values_mean});
	values.push({type: 'bar', name: 'max', x: tasks, y: values_max});
	if (plots_init)
	{
		Plotly.newPlot(graph, values,
			{
				title: "Tasks",
				width: 700,
				height: 350,
				barmode: "stack",
				margin: { t: 0 }
			},
			{
				showLink: false
			}
		);
	}else
	{
		graph.data = values;
		Plotly.redraw(graph);
	}
	if (plots_init)
		plots_init = false;
	*/
}

function ui() {
	if (json == null)
	{
		window.setTimeout(ui, 100);
		return;
	}
	if (init)
	{
		$("#project_name").text(json.info.project_name);
		init = false;
	}
	$("#console").append("<pre>" + json.log + "</pre>");
	tableActivitiesAPI.clear();
	tableActivitiesAPI.rows.add(json.activities);
	tableActivitiesAPI.draw();
	tableTasksAPI.clear();
	tableTasksAPI.rows.add(json.tasks);
	tableTasksAPI.draw();
	tableStatsAPI.clear();
	tableStatsAPI.rows.add(json.stats);
	tableStatsAPI.draw();
	if (selectedTab == "#tabs-graphs")
	{
		plots(json.stats);
	}
	window.setTimeout(ui, 500);
}

$(function() {
	$("button").button();

	bt_tools = $("#bt_tools").click(function(){
		$("#panel").dialog("open");
	});
	bt_tools.button("option", "disabled", false);

	bt_console = $("#bt_console").click(function(){
		$("#console").dialog("open");
	});
	bt_console.button("option", "disabled", false);

	$("#reset").button({icons: { primary: "ui-icon-refresh" }}).click(function() {
		$.post('/', {"action": "reset_stats"}, function(data, status) { }, "text");
	});
	$("#start").button({icons: { primary: "ui-icon-play" }}).click(notimplemented);
	$("#pause").button({icons: { primary: "ui-icon-pause" }}).click(notimplemented);
	$("#stop").button({icons: { primary: "ui-icon-stop" }}).click(notimplemented);
	$("#add").button({icons: { primary: "ui-icon-plus" }}).click(function(){
		addComponent();
	});
	$("#del").button({icons: { primary: "ui-icon-minus" }}).click(notimplemented);
	$("#mod").button({icons: { primary: "ui-icon-gear" }}).click(notimplemented);
	$("#xml").button({icons: { primary: "ui-icon-disk" }}).click(notimplemented);

	svg = document.getElementById("svg");
	s = Snap(svg);
	Snap.load("graph.svg", function(f) {
		$.each(f.selectAll("polygon"), function(idx, obj) {
			obj.parent().drag();
			obj.parent().click(function() {
//				$("#component").text(obj.parent().select("text").node.textContent);
			});
		});

		s.append(f.select("g"));
		$("#svg").css("width", $("#content").width() * 0.99);
		$("#svg").css("height", $("#content").height() - ($("#toolbar").height() * 1.5));
	});

/*
	$.get("/graph.svg", { }, function(svg) {
		$("#svg")[0].innerHTML = svg; 
		$("#svg").css("width", $("#content").width() * 0.99);
		$("#svg").css("height", $("#content").height() - ($("#toolbar").height() * 1.5));
	}, "text");
*/

	tableActivities = $("#table-activities").DataTable({
		"columns": [
			{ "data": "id" },
			{ "data": "active" },
			{ "data": "periodic" },
			{ "data": "period" },
			{ "data": "policy" }
		],
		"select": "single",
		"scrollY": "500px",
  		"scrollCollapse": true,
  		"paging": false
	}).on('select', function (e, dt, type, indexes) {

	});
	tableActivitiesAPI = $("#table-activities").dataTable().api();

	tableTasks = $("#table-tasks").DataTable({
		"columns": [
			{ "data": "name" },
			{ "data": "class" },
			{ "data": "type" },
			{ "data": "state" }
		],
		"select": "single",
		"scrollY": "500px",
  		"scrollCollapse": true,
  		"paging": false
	}).on('select', function (e, dt, type, indexes) {

	});
	tableTasksAPI = $("#table-tasks").dataTable().api();

	tableStats = $("#table-stats").DataTable({
		"columns": [
			{ "data": "name" },
			{ "data": "iterations" },
			{ "data": "time_mean" },
			{ "data": "time_stddev" },
			{ "data": "time_exec_mean" },
			{ "data": "time_exec_stddev" },
			{ "data": "time_min" },
			{ "data": "time_max" }
		],
		"select": "single",
		"scrollY": "500px",
  		"scrollCollapse": true,
  		"paging": false
	}).on('select', function (e, dt, type, indexes) {

	});
	tableStatsAPI = $("#table-stats").dataTable().api();

	selectedTab = "";
	$("#tabs").tabs({
		heightStyle: "fill",
		activate: function(event, ui) {
			selectedTab = ui.newPanel.selector;
		}
	});

	$("#panel").dialog({
		autoOpen: false,
		width: window.innerWidth * 0.8,
		height: window.innerHeight * 0.7,
		dialogClass: "small",
		show: {
			effect: "fold",
			duration: 250
		},
		hide: {
		effect: "drop",
			duration: 250
		},
		close: function (event, ui) {
			bt_tools.button("option", "disabled", false);
		},
		open: function (event, ui) {
			bt_tools.button("option", "disabled", true);
		}
	});

	$("#console").dialog({
		dialogClass: "small",
		autoOpen: false,
		width: window.innerWidth * 0.8,
		height: window.innerHeight * 0.7,
		show: {
			effect: "fold",
			duration: 250
		},
		hide: {
			effect: "drop",
			duration: 250
		},
		close: function (event, ui) {
			bt_console.button("option", "disabled", false);
		},
		open: function (event, ui) {
			bt_console.button("option", "disabled", true);
		}
	});

	if ("WebSocket" in window)
	{
		ws = new WebSocket("ws://" + document.location.host);
		ws.onopen = function() {

		};
		ws.onmessage = function(evt) {
			json = $.parseJSON(evt.data);
		};
		ws.onclose = function()
		{

		};
	}else
	{
		alert("WebSocket is not supported by your browser :(");
	}
	
	ui();

});
