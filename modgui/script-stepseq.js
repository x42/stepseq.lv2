function (event) {

	event.icon.find("[x42-role=seq-radio-nums]").click(function(){
		event.icon.find ("div.displayradio").removeClass ("selected");
		$(this).addClass ("selected");
		event.icon.find ("[x42-role=seq-note]").removeClass ("drum");
		event.icon.find ("[x42-role=seq-note]").removeClass ("note");
		event.icon.find ("[x42-role=seq-note]").addClass ("nums");
	});

	event.icon.find("[x42-role=seq-radio-note]").click(function(){
		event.icon.find ("div.displayradio").removeClass ("selected");
		$(this).addClass ("selected");
		event.icon.find ("[x42-role=seq-note]").removeClass ("drum");
		event.icon.find ("[x42-role=seq-note]").removeClass ("nums");
		event.icon.find ("[x42-role=seq-note]").addClass ("note");
	});

	event.icon.find("[x42-role=seq-radio-drum]").click(function(){
		event.icon.find ("div.displayradio").removeClass ("selected");
		$(this).addClass ("selected");
		event.icon.find ("[x42-role=seq-note]").removeClass ("note");
		event.icon.find ("[x42-role=seq-note]").removeClass ("nums");
		event.icon.find ("[x42-role=seq-note]").addClass ("drum");
	});

	event.icon.find("div.resetbutton.col").click(function(){
		var c = $(this).attr('grid-col');
		event.icon.find("[mod-role=input-control-port][grid-col="+c+"]").controlWidget('setValue', 0);
	});

	event.icon.find("div.resetbutton.row").click(function(){
		var r = $(this).attr('grid-row');
		event.icon.find("[mod-role=input-control-port][grid-row="+r+"]").controlWidget('setValue', 0);
	});

	event.icon.find("div.resetbutton.all").click(function(){
		event.icon.find("[mod-role=input-control-port][grid-row]").controlWidget('setValue', 0);
	});
}
