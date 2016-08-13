x42_seq_setdisplaymode = function (me, id, mode) {
	$("div.x42-stepseq" + id).find ("[x42-role=seq-radio-dpy]").removeClass ("selected");
	$(me).addClass ("selected");
	switch (mode) {
		case 'drum':
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").addClass ("drum");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("note");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("nums");
			break;
		case 'nums':
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("drum");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").addClass ("note");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("nums");
			break;
		case 'note':
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("drum");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").removeClass ("note");
			$("div.x42-stepseq" + id).find ("[x42-role=seq-note]").addClass ("nums");
			break;
		default:
			break;
	}
}
