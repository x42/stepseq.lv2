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

}
