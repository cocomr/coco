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

$(function() {
	$("button").button();
	$("#reset").button({icons: { primary: "ui-icon-refresh" }}).click(function() {
		$.post('/', {"action": "reset_stats"}, function(data, status) { }, "text");
	});
	$("#start").button({icons: { primary: "ui-icon-play" }});
	$("#pause").button({icons: { primary: "ui-icon-pause" }});
	$("#stop").button({icons: { primary: "ui-icon-stop" }});
	$("#add").button({icons: { primary: "ui-icon-plus" }}).click(function(){
		addComponent();
	});
	$("#del").button({icons: { primary: "ui-icon-minus" }});
	$("#mod").button({icons: { primary: "ui-icon-gear" }});
	$("#xml").button({icons: { primary: "ui-icon-disk" }}).click(function(){

	});

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
		$("#svg").css("width", s.getBBox().width + "pt");
		$("#svg").css("height", s.getBBox().height + "pt");
	});

	$.post("/info", { }, function(data) {
		$("#project_name").text(data.info.project_name);
	}, "json");

//	addComponent();

	table = $("#table-tasks").DataTable({
		"columns": [
			{ "data": "name" },
			{ "data": "class" },
			{ "data": "state" },
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
	
	tableapi = $("#table-tasks").dataTable().api();
	tableapi.clear();
	//tableapi.rows.add([{"name": "a", "class": "b", "state": "c", "iterations": "d"}]);
	tableapi.draw();

	bt_tools = $("#bt_tools").click(function(){
		$("#panel").dialog("open");
	});

	bt_console = $("#bt_console").click(function(){
		$("#console").dialog("open");
	});

	$("#tabs").tabs({heightStyle: "fill", activate: function(event, ui) {
		if (ui.newPanel.selector == "#tabs-menu")
		{
		}else if (ui.newPanel.selector == "#tabs-analysis") {

		}
	}});

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
		var ws = new WebSocket("ws://localhost:8080/");
		ws.onopen = function() {

		};
		ws.onmessage = function (evt)
		{
			$("#console").append("<p>" + evt.data + "</p>");
		};
		ws.onclose = function()
		{

		};
	}else
	{
		alert("WebSocket is not supported by your browser :(");
	}

});
