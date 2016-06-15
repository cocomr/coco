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

$(function() {
	$.post("/", { }, function(data) {
		$("#project_name").text(data.info.project_name);
		i = 0;
		for (g in data.graphs)
		{
			id = "graph" + i;
			$("#tabs-graphs").append('<div id="'+ id +'"></div>');
			graph = $('#' + id);
			Plotly.plot(graph[0], [
				{
					x: [1, 2, 3, 4, 5],
					y: [1, 2, 4, 8, 16]
				}
				],
				{
					title: g.title,
					width: 700,
					height: 350,
					margin: { t: 0 }
				}
			);
		}
	}, "json");

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
	var tableActivitiesAPI = $("#table-activities").dataTable().api();

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
	var tableTasksAPI = $("#table-tasks").dataTable().api();

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
	var tableStatsAPI = $("#table-stats").dataTable().api();

	$("#tabs").tabs({
		heightStyle: "fill",
		activate: function(event, ui) {
			if (ui.newPanel.selector == "#tabs-graphs")
			{

			}
		}
	});

	$("#panel").dialog({
		autoOpen: false,
		width: window.innerWidth * 0.8,
		height: window.innerHeight * 0.5,
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

	$("#console" ).dialog({
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
		var init = true;
		var ws = new WebSocket("ws://" + document.location.host);
		ws.onopen = function() {

		};
		ws.onmessage = function (evt)
		{
			json = jQuery.parseJSON(evt.data);
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
		};
		ws.onclose = function()
		{

		};
	}else
	{
		alert("WebSocket is not supported by your browser :(");
	}

});
