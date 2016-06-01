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
	//$.get('http://localhost:8080/graph.svg', {}, function(data, status) { svg = data; }, "text");

	$("button").button();
	$("#add").click(function(){ addComponent(); });
	s = Snap(document.getElementById("svg"));
	Snap.load("graph.svg", function(f) {
		$.each(f.selectAll("polygon"), function(idx, obj) {
			obj.parent().drag();
			obj.parent().click(function(){ $("#component").text(obj.parent().select("text").node.textContent); });
		});
		s.append(f.select("g"));
	});
	
	//addComponent();
	
	table = $("#datatable").DataTable({
		"ajax": {
			"url": "/",
			"type": "POST"
		},
		"columns": [
			{ "data": "name" },
			{ "data": "state" },
			{ "data": "iterations" },
			{ "data": "time_mean" },
			{ "data": "time_stddev" },
			{ "data": "time_exec_mean" },
			{ "data": "time_exec_stddev" },
			{ "data": "time_min" },
			{ "data": "time_max" }
		],
		"select": "single"
	});
	
	setInterval(function(){ table.ajax.reload(); }, 1000);
});
