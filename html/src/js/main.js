import '../scss/main.scss';

const MAX_STRING_LENGTH = 64;
const EditMode = {
	Animate: "output",
	Frames: "frames",
	Sequences: "sequences",
	Settings: "settings",
	Script: "script_editor"
};
var editMode = EditMode.Animate;
var scripteditor;

// Servo settings
const newServoSetting = {
	srvuseerr: false,
	srvmin: 1000,
	srvmax: 2000,
	srverr: 1000,
	srvgrp: 0
};
const newServosSetting = new Map();
newServosSetting.set('1', {srvena: true, srvmin: 1000, srvmax: 2000, srvuseerr: false, srverr: 1000});
newServosSetting.set('2', {srvena: true, srvmin: 1000, srvmax: 2000, srvuseerr: false, srverr: 1000});
newServosSetting.set('3', {srvena: true, srvmin: 1000, srvmax: 2000, srvuseerr: false, srverr: 1000});
newServosSetting.set('4', {srvena: true, srvmin: 1000, srvmax: 2000, srvuseerr: false, srverr: 1000});
// Servo frames
const newServoFrame = {
	srvena: false,
	srvpos: 0,
	srvdur: 50,
	srveas: 0
};
const newServosFrame = new Map();
newServosFrame.set('1', {srvena: true, srvnam: '1', srvpos: 80, srvdur: 10, srveas: 5});
newServosFrame.set('2', {srvena: true, srvnam: '2', srvpos: 20, srvdur: 20, srveas: 4});
newServosFrame.set('3', {srvena: true, srvnam: '3', srvpos: 90, srvdur: 30, srveas: 6});
newServosFrame.set('4', {srvena: true, srvnam: '4', srvpos: 50, srvdur: 40, srveas: 7});

var timeliner = null;
var sequences = [];
var savedSequences = new Map();
var savedSequenceOrder = "";
var savedSequenceOrderArray = [];
var savedSettings = "";
var activeSequence = null;
var playing = false;
var activeSequenceIndex = sequences.length - 1;
var servoSettings = newServosSetting;
var frames = [ newServosFrame ];
var frameID = 1;
var currentFrame = frames.length-1;

// pretend to be jquery
function $(id) {
	let result = document.getElementById(id);
	if (result == null) {
		console.log('Not found:',id);
	}
	return result;
}

function getStyleRuleWidth(selectorText) {
	let sheets = document.styleSheets;
	let sheet, rules, rule;
	let i, j, k, l, iLen, jLen;

	for (i=0, iLen=sheets.length; i<iLen; i++) {
		sheet = sheets[i];
		if (sheet.cssRules) {
			rules = sheet.cssRules;
			for (j=0, jLen=rules.length; j<jLen; j++) {
				rule = rules[j];
				if (rule.selectorText == selectorText) {
					return parseInt(rule.style.width.slice(0, -2));
				}
			}
		// IE model
		} else if (sheet.rules) {
			rules = sheet.rules;
			for (k=0, kLen=rules.length; k<kLen; k++) {
				rule = rules[k];
				if (rule.selectorText == selectorText) {
					return parseInt(rule.style.width.slice(0, -2));
				}
			}
		}
	}
	return 0;
}

function modifyStyleRuleWidth(selectorText, newWidth) {
	let sheets = document.styleSheets;
	let sheet, rules, rule;
	let i, j, k, l, iLen, jLen;

	for (i=0, iLen=sheets.length; i<iLen; i++) {
		sheet = sheets[i];
		if (sheet.cssRules) {
			rules = sheet.cssRules;
			for (j=0, jLen=rules.length; j<jLen; j++) {
				rule = rules[j];
				if (rule.selectorText == selectorText) {
					rule.style.width = newWidth+"px";
				}
			}
		// IE model
		} else if (sheet.rules) {
			rules = sheet.rules;
			for (k=0, kLen=rules.length; k<kLen; k++) {
				rule = rules[k];
				if (rule.selectorText == selectorText) {
					rule.style.width = newWidth+"px";
				}
			}
		}
	}
}

function addOutputRow(){
	let table = $('output');
	let rowCount = table.rows.length;
	let cellCount = table.rows[0].cells.length;
	let row = table.insertRow(rowCount);
	for(let i =0; i < cellCount; i++) {
		let cell = 'cell'+i;
		cell = row.insertCell(i);
		let copycel = $('col'+i).innerHTML;
		cell.innerHTML=copycel;
		cell.onchange=updateServoValue;
	}
}

function addFramesRow(){
	let table = $('frames');
	let rowCount = table.rows.length;
	let cellCount = table.rows[0].cells.length;
	let row = table.insertRow(rowCount);
	for(let i =0; i < cellCount; i++) {
		let cell = 'cell'+i;
		cell = row.insertCell(i);
		let copycel = $('frcol'+i).innerHTML;
		cell.innerHTML=copycel;
		cell.onchange=updateFrameValue;
	}
}

function addSettingsRow(){
	let table = $('settings');
	let rowCount = table.rows.length;
	let cellCount = table.rows[0].cells.length;
	let row = table.insertRow(rowCount);
	for(let i =0; i < cellCount; i++){
		let cell = 'cell'+i;
		cell = row.insertCell(i);
		let copycel = $('setcol'+i).innerHTML;
		cell.innerHTML=copycel;
		cell.onchange=updateSettingsValue;
	}
}

function addSequencesRow(){
	let table = $('sequences');
	let rowCount = table.rows.length;
	let cellCount = table.rows[0].cells.length;
	let row = table.insertRow(rowCount);
	for(let i =0; i < cellCount; i++){
		let cell = 'cell'+i;
		cell = row.insertCell(i);
		let copycel = $('seqcol'+i).innerHTML;
		cell.innerHTML=copycel;
		cell.onchange=updateSequenceValue;
	}
}

function uuidv4() {
	return ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, c =>
		(c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> c / 4).toString(16)
	);
}

function loadSettings() {
	let table = $('settings');
	rightSizeTable(table, servoSettings.length, addSettingsRow);
	for (let i = 0; i < servoSettings.length; i++) {
		const row = table.rows[i+1];
		const srvval = servoSettings[i];
		for (const [key, value] of Object.entries(srvval)) {
			let el = row.querySelector("#"+key);
			el.checked = value;
			el.value = value;
		}
	}
}

function msToTime(duration) {
  let milliseconds = (duration % 1000),
    seconds = Math.floor((duration / 1000) % 60),
    minutes = Math.floor((duration / (1000 * 60)) % 60),
    hours = Math.floor((duration / (1000 * 60 * 60)) % 24);

  if (hours > 0) {
  	hours = String(hours).padStart(2, '0')+":";
  } else {
  	hours = "";
  }
  minutes = String(minutes).padStart(2, '0');
  seconds = String(seconds).padStart(2, '0');
  milliseconds = String(milliseconds).padStart(3, '0');

  return hours + minutes + ":" + seconds + "." + milliseconds;
}

function loadSequences() {
	let table = $('sequences');
	rightSizeTable(table, sequences.length, addSequencesRow);
	for (let i = 0; i < sequences.length; i++) {
		const row = table.rows[i+1];
		const seqval = sequences[i];
		let cellCount = 0;
		for (const [key, value] of Object.entries(seqval)) {
			let el = row.querySelector("#"+key);
			if (el != null) {
				el.checked = value;
				el.value = value;
				cellCount++;
			}
		}
		let selel = row.querySelector("#compDur");
		if (selel != null) {
			selel.value = msToTime(getSequenceDuration(seqval, 0));
		}
		selel = row.querySelector("#sel");
		if (selel != null) {
			selel.checked = (activeSequence != null && seqval.seqID == activeSequence.seqID);
		}
	}
}

function getServoName(index) {
	if (index > 0 && index < servoSettings.length) {
		return servoSettings[index-1].srvnam;
	}
	return "Servo"+(index);
}

function rightSizeTable(table, len, adder)
{
	while (table.rows.length - 1 > len) {
		table.deleteRow(table.rows.length - 1);
	}
	for (let i = table.rows.length-1; i < len; i++) {
		adder();
	}
}

function loadFrame() {
	let table = $('output');
	let rowCount = table.rows.length;
	const frame = frames[currentFrame];
	rightSizeTable(table, servoSettings.length, addOutputRow);
	for (let i = 1; i < rowCount; i++) {
		const row = table.rows[i];
		const srvidx = i.toString();
		let srvnam = row.querySelector("#srvnam");
		srvnam.value = getServoName(i);
		let srvval = frame[srvidx];
		if (!frame.hasOwnProperty(srvidx)) {
			/* add empty */
			srvval = newServoFrame;
		}
		for (const [key, value] of Object.entries(srvval)) {
			let el = row.querySelector("#"+key);
			el.checked = value;
			el.value = value;
		}
	}
	let frameIndicator = $('navbarinfo');
	frameIndicator.innerHTML = ""+(currentFrame+1)+" ("+(frames.length)+")";
	// const outputTable = $("output");
	// outputTable.hidden = false; //removeAttribute("hidden");
}

function dupFrameAtIndex(index) {
	let clearServoEnabled = (index == frames.length-1);
	if (frames.length < 99) {
		let newFrame = JSON.parse(JSON.stringify(frames[index]));
		newFrame.frnam = (frameID++).toString();
		newFrame.frbeg = "";
		newFrame.frend = "";
		if (clearServoEnabled) {
			for (let i = 0; i < servoSettings.length; i++) {
				const srvidx = i.toString();
				if (newFrame.hasOwnProperty(srvidx)) {
					let servo = newFrame[srvidx];
					servo.srvena = false;
				}
			}
		}
		if (currentFrame == frames.length-1) {
			frames.push(newFrame);
		} else {
			frames.splice(currentFrame, 0, newFrame);
		}
		if (index <= currentFrame)
			currentFrame++;
		return true;
	}
	return false;
}

function addEmptyFrame() {
	if (frames.length < 99) {
		frames.push({
			frnam: (frameID++).toString(),
			frbeg: "",
			frend: "",
			frdur: 50
		});
		currentFrame = frames.length-1;
		return true;
	}
	return false;
}

function checkUniqueSequenceName(name) {
	for (let i = 0; i < sequences.length; i++) {
		if (name == sequences[i].seqNam)
			return false;
	}
	return true;
}

function getNumberAtEnd(str) {
  if (/[0-9]+$/.test(str)) {
    return str.match(/[0-9]+$/)[0];
  }
  return null;
}

function dupSequenceAtIndex(index) {
	if (index < sequences.length) {
		let newSeq = JSON.parse(JSON.stringify(sequences[index]));
		newSeq.seqID = uuidv4();
		let name = newSeq.seqNam;
		let numEnd = getNumberAtEnd(name);
		if (numEnd != null) {
			name = name.substring(0, name.length - numEnd.length);
		} else {
			name = name + " #";
		}
		for (let ni = 0; ; ni++) {
			let checkName = name + (sequences.length + ni);
			if (checkUniqueSequenceName(checkName)) {
				name = checkName;
				break;
			}
		}
		newSeq.seqNam = name;
		if (activeSequenceIndex == sequences.length-1) {
			sequences.push(newSeq);
			selectSequenceAtIndex(sequences.length-1);
		} else {
			sequences.splice(activeSequenceIndex, 0, newSeq);
			selectSequenceAtIndex(activeSequenceIndex+1);
		}
		return true;
	}
	return false;
}

function selectSequenceAtIndex(index) {
	if (index < sequences.length) {
		activeSequenceIndex = index;
		activeSequence = sequences[activeSequenceIndex];
		frames = activeSequence.seqframes;
		currentFrame = frames.length-1;
		frameID = frames.length+1;

		loadTimeline(frames);
	}
}

function add_button(index) {
	if (editMode == EditMode.Animate)
	{
		if (dupFrameAtIndex(currentFrame))
			loadFrame();
	}
	else if (editMode == EditMode.Frames)
	{
		let added = false;
		let table = $('frames');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			let row = table.rows[i+1];
			let sel = row.querySelector("#sel");
			if (sel.checked)
			{
				if (dupFrameAtIndex(i))
				{
					added = true;
					refreshFrames();
					break;
				}
			}
		}
		if (!added && addEmptyFrame()) {
			refreshFrames();
		}
	}
	else if (editMode == EditMode.Sequences)
	{
		let added = false;
		let table = $('sequences');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			let row = table.rows[i+1];
			let sel = row.querySelector("#sel");
			if (sel.checked)
			{
				if (dupSequenceAtIndex(i))
				{
					added = true;
					break;
				}
			}
		}
		if (!added) {
			addEmptySequence();
			selectSequenceAtIndex(sequences.length-1);
		}
		loadSequences();
	}
}

function edit_button() {
	if (editMode == EditMode.Frames)
	{
		let table = $('frames');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				currentFrame = i;
				showAnimate();
				break;
			}
		}
	}
	else if (editMode == EditMode.Sequences)
	{
		let table = $('sequences');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				selectSequenceAtIndex(i);
				showAnimate();
				break;
			}
		}
	}
}

function moveUpAndDown(mode, moveUp)
{
	let scrolled = false;
	let table = $(mode);
	let rowCount = table.rows.length;
	if (moveUp)
	{
		for (let i = 1; i < table.rows.length; i++)
		{
			let row = table.rows[i];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				if (i > 1)
				{
					row.parentNode.insertBefore(row, table.rows[i - 1]);
					if (editMode == EditMode.Frames) {
						let prevFrame = frames[i-2];
						frames[i-2] = frames[i-1];
						frames[i-1] = prevFrame;
					}
					else if (editMode == EditMode.Sequences) {
						let prevSeq = sequences[i-2];
						sequences[i-2] = sequences[i-1];
						sequences[i-1] = prevSeq;
					}
					if (!scrolled) {
						table.rows[i - 1].scrollIntoView({
						    behavior: 'smooth',
						    block: 'center'
						});
						scrolled = true;
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		let lastRow = null;
		for (let i = table.rows.length-1; i >= 1; i--)
		{
			const row = table.rows[i];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				if (i < rowCount - 1)
				{
					row.parentNode.insertBefore(table.rows[i + 1], row);
					lastRow = table.rows[i + 1];
					if (editMode == EditMode.Frames) {
						let nextFrame = frames[i];
						frames[i] = frames[i-1];
						frames[i-1] = nextFrame;
					} else if (editMode == EditMode.Sequences) {
						let nextSeq = sequences[i];
						sequences[i] = sequences[i-1];
						sequences[i-1] = nextSeq;
					}
				}
				else
				{
					break;
				}
			}
		}
		if (lastRow != null) {
			lastRow.scrollIntoView({
			    behavior: 'smooth',
			    block: 'center'
			});
		}
	}
	if (editMode == EditMode.Frames) {
		updateCurrentFrame();
	}
}

function up_button() {
	if (editMode == EditMode.Frames ||
			editMode == EditMode.Sequences)
	{
		moveUpAndDown(editMode, true);
	}
}

function down_button() {
	if (editMode == EditMode.Frames ||
			editMode == EditMode.Sequences)
	{
		moveUpAndDown(editMode, false);
	}
}

function deleteFrameAtIndex(index) {
	if (frames.length > 1) {
		frames.splice(index, 1);
		if (index <= currentFrame)
		{
			if (--currentFrame < 0)
				currentFrame = 0;
		}
		return true;
	}
	return false;
}

function numberOfSelectedFrames() {
	let count = 0;
	let table = $('frames');
	let rowCount = table.rows.length-1;
	for (let i = 0; i < rowCount;)
	{
		const row = table.rows[i+1];
		const sel = row.querySelector("#sel");
		if (sel.checked)
		{
			count++;
		}
		i++;
	}
	return count;
}

function numberOfSelectedSequences() {
	let count = 0;
	let table = $('sequences');
	let rowCount = table.rows.length-1;
	for (let i = 0; i < rowCount;)
	{
		const row = table.rows[i+1];
		const sel = row.querySelector("#sel");
		if (sel.checked)
		{
			count++;
		}
		i++;
	}
	return count;
}

function delete_button() {
	if (editMode == EditMode.Animate)
	{
		if (deleteFrameAtIndex(currentFrame))
		{
			loadFrame();
		}
	}
	else if (editMode == EditMode.Frames)
	{
		const count = numberOfSelectedFrames();
		if (count > 1 && !confirm("You are about to delete "+count+" frames. Are you sure?"))
			return;
		let table = $('frames');
		let rowCount = table.rows.length-1;
		let firstSelectedIndex = -1;
		for (let i = 0; i < rowCount;)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				if (firstSelectedIndex == -1)
					firstSelectedIndex = i;
				frames.splice(i, 1);
				if (rowCount == 1)
					break;
				table.deleteRow(i+1);
				rowCount = table.rows.length-1;
			}
			else
			{
				i++;
			}
		}
		if (frames.length == 0) {
			// Create new blank sequence
			addEmptyFrame();
		}
		currentFrame = ((firstSelectedIndex < frames.length) ?
			firstSelectedIndex : frames.length-1);
		const row = table.rows[currentFrame+1];
		const sel = row.querySelector("#sel");
		sel.checked = true;
		if (frames.length == 1) {
			refreshFrames();
		}
	}
	else if (editMode == EditMode.Sequences)
	{
		const count = numberOfSelectedSequences();
		if (count == 0)
			return;
		else if (count == 1 && !confirm("You are about to delete the selected sequence. Are you sure?"))
			return;
		else if (count > 1 && !confirm("You are about to delete "+count+" sequences. Are you sure?"))
			return;
		let table = $('sequences');
		let rowCount = table.rows.length-1;
		let firstSelectedIndex = -1;
		let deleteSequences = [];
		for (let i = 0; i < rowCount; ) {
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked) {
				if (firstSelectedIndex == -1)
					firstSelectedIndex = i;
				let seqid = sequences[i].seqID;
				if (savedSequences.has(seqid))
					deleteSequences.push(seqid);
				sequences.splice(i, 1);
				if (rowCount == 1)
					break;
				table.deleteRow(i+1);
				rowCount = table.rows.length-1;
			}
			else
			{
				i++;
			}
		}
		if (deleteSequences.length > 0)
			postDeleteSequences(deleteSequences);
		if (sequences.length == 0) {
			// Create new blank sequence
			addEmptySequence();
			frames = activeSequence.seqframes;
			currentFrame = frames.length-1;
			frameID = frames.length+1;
		} else {
			selectSequenceAtIndex((firstSelectedIndex < sequences.length) ? firstSelectedIndex : sequences.length-1);
		}
		const row = table.rows[activeSequenceIndex+1];
		const sel = row.querySelector("#sel");
		sel.checked = true;
		if (sequences.length == 1) {
			loadSequences();
		}
	}
}

function postDeleteSequences(deleteSequences) {
	if (deleteSequences.length > 0) {
		let seqid = deleteSequences[0];
		changedSequences.shift();
		const searchParams = new URLSearchParams();
		searchParams.append('id', seqid);
		fetch('/api/delete?'+searchParams)
			.then(response => {
				if (!response.ok) {
					alert("Failed to delete sequence: "+seqid);
				} else {
					savedSequences.delete(seqid);
				}
				postDeleteSequences(deleteSequences);
			})
			.catch(function(error) {
				console.log('Error:',error);
				alert("Failed to upload sequence: "+seqid);
			});
	} else {
		uploadSequenceOrder();
	}
}

function random_frames_max() {
	while (frames.length < 99)
	{
		let newFrame = {};
		newFrame.frnam = uuidv4();
		newFrame.frbeg = uuidv4();
		newFrame.frend = uuidv4();
		newFrame.frdur = 0;//Math.round(Math.random() * 200);
		for (let i = 0; i < servoSettings.length; i++)
		{
			const srvidx = i.toString();
			/* add empty */
			let servo = {...newServoFrame};
			servo.srvena = true;
			servo.srvpos = Math.round(Math.random() * 1000);
			servo.srvdur = Math.round(Math.random() * 200);
			servo.srveas = Math.round(Math.random() * 31);
			newFrame[srvidx] = servo;
		}
		frames.push(newFrame);
	}
}

function encodeJSON(type,data) {
	let typeInt = -1;
	let compress = false;
	if (type == 'sequence') {
		typeInt = 0;
		compress = true;
	}
	else if (type == 'sequences') {
		typeInt = 1;
		compress = true;
	}
	else if (type == 'settings') {
		typeInt = 2;
	}
	if (typeInt != -1)
	{
		const json = orderedStringify(data);
		let utfstr = new Uint8Array(toUTF8Array(json));
		let utfstr_mem = Module._malloc(utfstr.length + 1);
		Module.HEAPU8.set(utfstr, utfstr_mem);
		let result = (compress) ? Module._deflateJSON(typeInt, utfstr_mem, utfstr.length) :
			Module._encodeJSON(typeInt, utfstr_mem, utfstr.length);
		Module._free(utfstr_mem);
		if (result != null) {
			let encodedBytes = UTF8ToString(result);
			Module._free(result);
			return encodedBytes;
		}
	}
	else
	{
		console.log('bad encode type:',type);
	}
	return null;
}

function getSequenceDuration(seq,startFrame) {
	let seqDur = 0;
	let seqFrames = seq.seqframes;
	for (let fi = startFrame; fi < seqFrames.length; fi++) {
		let seqFrame = seqFrames[fi];
		let frameDur = Number(seqFrame.frdur);
		if (frameDur == 0) {
			for (let si = 0; si < servoSettings.length; si++)
			{
				let srvidx = (si+1).toString();
				if (seqFrame.hasOwnProperty(srvidx)) {
					let srv = seqFrame[srvidx];
					if (frameDur < Number(srv.srvdur))
						frameDur = Number(srv.srvdur);
				}
			}
		}
		seqDur += frameDur;
	}
	return seqDur * 10;
}

function getSequenceDurationID(id,startFrame) {
	let seqDur = 0;
	for (let si = 0; si < sequences.length; si++) {
		if (id == sequences[si].seqID) {
			seqDur = getSequenceDuration(sequences[si], startFrame);
			break;
		}
	}
	return seqDur;
}

function play_sequence(id) {
	const searchParams = new URLSearchParams();
	searchParams.append('id', id);
	let startFrame = 0;
	if (editMode == EditMode.Animate) {
		startFrame = currentFrame;
		searchParams.append('num', startFrame);
	}
	let duration = getSequenceDurationID(id,startFrame);
	fetch('/api/play?'+searchParams)
		.then(response => {
			if (response.ok) {
				playing = true;
				$("playSeq").hidden = true;
				$("stopSeq").hidden = false;
				console.log('duration:',duration);
				setTimeout(() => {
					$("playSeq").hidden = false;
					$("stopSeq").hidden = true;
					playing = false;
				}, duration)
			}
		})
		.catch(function(error) {
			console.log(error);
		});
}

function stop_button() {
	fetch('/api/stop')
		.then(response => {
			if (response.ok) {
				$("playSeq").hidden = false;
				$("stopSeq").hidden = true;
				playing = false;
			}
		})
		.catch(function(error) {
			console.log(error);
		});
}

function goto_frame() {
	if (editMode == EditMode.Animate) {
		let frameNum = parseInt(prompt("Enter frame number (1-"+(frames.length)+")", ""+(currentFrame+1)));
		if (frameNum >= 1 && frameNum <= frames.length) {
			currentFrame = frameNum-1;
			loadFrame();
		}
	}
}

function saveSequence(seq, play,errorMsg) {
	if (seq != null) {
		let result = encodeJSON('sequence',seq);
		if (result == null) {
			console.log("Internal error. Failed to encode sequence.")
			return;
		}
		let sequenceChanged = false;
		if (savedSequences.has(seq.seqID)) {
			let stored = savedSequences.get(seq.seqID);
			if (stored != result) {
				sequenceChanged = true;
			}
		} else {
			sequenceChanged = true;
		}
		if (sequenceChanged) {
			saveSettings();
			postSequence(seq.seqID, result)
				.then((response) =>  {
					if (!response.ok) {
						alert(errorMsg);
					} else {
						savedSequences.set(seq.seqID, result);
						if (play)
							play_sequence(seq.seqID);
					}
				})
				.catch((error) => {
					alert(errorMsg);
				});
		} else if (play) {
			play_sequence(seq.seqID);
		}
	}
}

function saveSequences() {
	let table = $('sequences');
	let rowCount = table.rows.length-1;
	saveSettings();
	let changedSequences = [];
	for (let i = 0; i < rowCount; i++) {
		let seq = sequences[i];
		let result = encodeJSON('sequence',seq);
		if (result == null) {
			console.log("Internal error. Failed to encode sequence.")
			return;
		}
		let sequenceChanged = false;
		if (savedSequences.has(seq.seqID)) {
			let stored = savedSequences.get(seq.seqID);
			if (stored != result) {
				sequenceChanged = true;
			}
		} else {
			sequenceChanged = true;
		}
		if (sequenceChanged) {
			changedSequences.push({
				id: seq.seqID,
				data: result
			});
		}
	}
	postSequences(changedSequences);
}

function postSequences(changedSequences) {
	if (changedSequences.length > 0) {
		let seq = changedSequences[0];
		changedSequences.shift();
		postSequence(seq.id, seq.data)
			.then((response) =>  {
				if (!response.ok) {
					alert("Failed to upload sequence: "+seq.id);
				} else {
					savedSequences.set(seq.id, seq.data);
				}
				postSequences(changedSequences);
			})
			.catch((error) => {
				console.log("Error:",error);
				alert("Failed to upload sequence: "+seq.id);
			});
	} else {
		uploadSequenceOrder();
	}
}

function play_button() {
	saveSequence(activeSequence, true, "Failed to upload sequence. Unable to play");
}

function prevFrame() {
	if (currentFrame > 0)
	{
		currentFrame--;
		loadFrame();
	}
}

function nextFrame() {
	if (currentFrame+1 < frames.length)
	{
		currentFrame++;
		loadFrame();
	}
}

function refreshFrames() {
	const table = $('frames');
	rightSizeTable(table, frames.length, addFramesRow);
	for (let i = 0; i < frames.length; i++) {
		const row = table.rows[i+1];
		const frame = frames[i];
		const sel = row.querySelector("#sel");
		sel.checked = (currentFrame == i);
		for (const prop in frame) {
			if (prop.startsWith('fr'))
			{
				const value = frame[prop]
				let el = row.querySelector("#"+prop);
				el.checked = value;
				el.value = value;
			}
		}
	}
	frameID = frames.length+1;
	table.rows[currentFrame].scrollIntoView({
	    behavior: 'smooth',
	    block: 'center'
	});
}

function updateCurrentFrame()
{
	if (editMode == EditMode.Frames) {
		const table = $('frames');
		const rowCount = table.rows.length;
		for (let i = 1; i < rowCount; i++)
		{
			const row = table.rows[i];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				currentFrame = i-1;
				break;
			}
		}
	}
}

function showAnimate() {
	updateCurrentFrame();
	editMode = EditMode.Animate;
	$("output").hidden = false;
	if (timeliner != null)
		timeliner.hide();
	$("frames").hidden = true;
	$("sequences").hidden = true;
	$("settings").hidden = true;
	$("script_editor").hidden = true;
	$("add_button").hidden = false;
	$("edit_button").hidden = true;
	$("up_button").hidden = true;
	$("down_button").hidden = true;
	$("del_button").hidden = false;
	$("prevFrame").hidden = false;
	$("nextFrame").hidden = false;
	$("navbarinfo").hidden = false;
	$("sync_script").hidden = true;
	$("upload_button").hidden = true;
	loadFrame();
}

function showTimeline() {
	updateCurrentFrame();
	editMode = EditMode.Animate;
	$("output").hidden = true;
	$("frames").hidden = true;
	if (timeliner != null)
		timeliner.show();
	$("sequences").hidden = true;
	$("settings").hidden = true;
	$("script_editor").hidden = true;
	$("add_button").hidden = false;
	$("edit_button").hidden = true;
	$("up_button").hidden = true;
	$("down_button").hidden = true;
	$("del_button").hidden = false;
	$("prevFrame").hidden = false;
	$("nextFrame").hidden = false;
	$("navbarinfo").hidden = false;
	$("sync_script").hidden = true;
	$("upload_button").hidden = true;
	loadFrame();
}

function showFrames() {
	updateCurrentFrame();
	editMode = EditMode.Frames;
	$("output").hidden = true;
	if (timeliner != null)
		timeliner.hide();
	$("frames").hidden = false;
	$("sequences").hidden = true;
	$("settings").hidden = true;
	$("script_editor").hidden = true;
	$("add_button").hidden = false;
	$("edit_button").hidden = false;
	$("up_button").hidden = false;
	$("down_button").hidden = false;
	$("del_button").hidden = false;
	$("prevFrame").hidden = true;
	$("nextFrame").hidden = true;
	$("navbarinfo").hidden = true;
	$("sync_script").hidden = true;
	$("upload_button").hidden = false;
	refreshFrames();
}

function showSequences() {
	updateCurrentFrame();
	editMode = EditMode.Sequences;
	$("output").hidden = true;
	if (timeliner != null)
		timeliner.hide();
	$("frames").hidden = true;
	$("sequences").hidden = false;
	$("settings").hidden = true;
	$("script_editor").hidden = true;
	$("add_button").hidden = false;
	$("edit_button").hidden = false;
	$("up_button").hidden = false;
	$("down_button").hidden = false;
	$("del_button").hidden = false;
	$("prevFrame").hidden = true;
	$("nextFrame").hidden = true;
	$("navbarinfo").hidden = true;
	$("sync_script").hidden = true;
	$("upload_button").hidden = false;
	loadSequences();
}

function showSettings() {
	updateCurrentFrame();
	editMode = EditMode.Settings;
	$("output").hidden = true;
	if (timeliner != null)
		timeliner.hide();
	$("frames").hidden = true;
	$("sequences").hidden = true;
	$("settings").hidden = false;
	$("script_editor").hidden = true;
	$("add_button").hidden = true;
	$("up_button").hidden = true;
	$("down_button").hidden = true;
	$("edit_button").hidden = true;
	$("del_button").hidden = true;
	$("prevFrame").hidden = true;
	$("nextFrame").hidden = true;
	$("navbarinfo").hidden = true;
	$("sync_script").hidden = true;
	$("upload_button").hidden = false;
	loadSettings();
}

function showScript() {
	updateCurrentFrame();
	editMode = EditMode.Script;
	$("output").hidden = true;
	if (timeliner != null)
		timeliner.hide();
	$("frames").hidden = true;
	$("sequences").hidden = true;
	$("settings").hidden = true;
	$("script_editor").hidden = false;
	$("add_button").hidden = true;
	$("edit_button").hidden = true;
	$("up_button").hidden = true;
	$("down_button").hidden = true;
	$("del_button").hidden = true;
	$("prevFrame").hidden = true;
	$("nextFrame").hidden = true;
	$("navbarinfo").hidden = true;
	$("sync_script").hidden = false;
	$("upload_button").hidden = false;
}

function loadFromFile(contents) {
	if (editMode == EditMode.Animate ||
			editMode == EditMode.Frames)
	{
		let badData = false
		let added = false
		try {
			let sequence = JSON.parse(contents);
			if (sequence.hasOwnProperty('seqID') &&
					sequence.hasOwnProperty('seqNam') &&
					sequence.hasOwnProperty('seqframes'))
			{
				console.log('seq2:',sequence);
				for (const frame of sequence.seqframes) {
						console.log('frame:',frame);
					if (frame.hasOwnProperty('frdur') &&
							frame.hasOwnProperty('frnam') &&
							frame.hasOwnProperty('frbeg') &&
							frame.hasOwnProperty('frend'))
					{
						if (frames.length < 99) {
							let newFrame = JSON.parse(JSON.stringify(frame));
							newFrame.frnam = !isNaN(frame.frnam) ? (frameID++).toString() : frame.frnam;
							newFrame.frbeg = frame.frbeg;
							newFrame.frend = frame.frend;
							newFrame.frdur = frame.frdur;
							console.log('newFrame:',newFrame);
							frames.push(newFrame);
							added = true;
						}
					} else {
						badData = true;
						break;
					}
				}
			} else {
				badData = true;
			}
		} catch(e) {
			console.log('error:',e);
			badData = true;
		}
		if (badData) {
			alert("Invalid sequence file.");
		} else if (added) {
			refreshFrames()
		}
	} else if (editMode == EditMode.Sequences) {
		let badData = false
		try {
			let sequences = JSON.parse(contents);
			for (const sequence of sequences) {
				let result = encodeJSON('sequence',sequence);
				if (result != null)
				{
					postSequence(sequence.seqID, result)
						.then((response) =>  {
							if (!response.ok) {
								alert("Failed to upload sequence: "+sequence.seqNam);
							} else {
								savedSequences.set(activeSquence.seqID, result);
							}
						})
						.catch((error) => {
							alert("Failed to upload sequence: "+sequence.seqNam);
						});
				} else {
					badData = true;
				}
			}
		} catch(e) {
			badData = true;
		}
		if (badData) {
			alert("Invalid sequences file.");
		}
	} else if (editMode == EditMode.Settings) {
		let badData = false
		try {
			let settings = JSON.parse(contents);
			for (let i = 0; i < settings.length; i++) {
				let setting = settings[i];
				if (!setting.hasOwnProperty('srvena') ||
						!setting.hasOwnProperty('srverr') ||
						!setting.hasOwnProperty('srvmax') ||
						!setting.hasOwnProperty('srvmin') ||
						!setting.hasOwnProperty('srvnam') ||
						!setting.hasOwnProperty('srvuseerr'))
				{
					console.log('bad:',setting);
					// Missing fields
					badData = true;
					break;
				}
			}
			if (!badData) {
				// Filter out any other unwanted fields
				servoSettings = settings.map(
					({srvena, srverr, srvmax, srvmin, srvnam, srvuseerr}) =>
						({srvena, srverr, srvmax, srvmin, srvnam, srvuseerr}));
				loadSettings();
				let result = encodeJSON('settings',servoSettings);
				if (result != null) {
					postSettings(result)
						.then((response) =>  {
							if (!response.ok)
							{
									alert("Failed to upload settings.");
							}
						})
						.catch((error) => {
							alert("Failed to upload settings.");
						});
				}
			}
		} catch(e) {
			badData = true;
		}
		if (badData) {
			alert("Invalid servo settings.");
		}
	} else if (editMode == EditMode.Script) {
		scripteditor.setValue(contents);
		scripteditor.getSession().setAnnotations([{}]);
	}
}

function openFile(func) {
	let readFile = function(e) {
		let file = e.target.files[0];
		if (!file) {
			return;
		}
		let reader = new FileReader();
		reader.onload = function(e) {
			let contents = e.target.result;;
			fileInput.func(contents);
			document.body.removeChild(fileInput);
		}
		reader.readAsText(file);
	}
	var fileInput = document.createElement("input");
	fileInput.type='file';
	fileInput.style.display='none';
	fileInput.onchange=readFile;
	fileInput.func=func;
	document.body.appendChild(fileInput);
	let eventMouse = document.createEvent("MouseEvents");
	eventMouse.initMouseEvent("click", true, false, window,
				0, 0, 0, 0, 0, false, false, false, false, 0, null);
	fileInput.dispatchEvent(eventMouse);
}

function saveToJSONFile() {
	if (editMode == EditMode.Animate) {
		let blob = new Blob([JSON.stringify(activeSequence, undefined, 4)], {type: "application/json;charset=utf-8"});
		saveAs(blob, activeSequence.seqNam + ".json");
	} else if (editMode == EditMode.Frames) {
		let selectedFrames = [];
		let table = $('frames');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				selectedFrames.push(frames[i]);
			}
		}
		if (selectedFrames.length != 0) {
			let subsequence = {
				seqID: activeSequence.seqID,
				seqNam: activeSequence.seqNam,
				seqframes: selectedFrames
			};
			let blob = new Blob([JSON.stringify(subsequence, undefined, 4)], {type: "application/json;charset=utf-8"});
			saveAs(blob, subsequence.seqNam + ".json");
		} else {
			alert("No frames selected")
		}
	} else if (editMode == EditMode.Sequences) {
		let selectedSequences = [];
		let table = $('sequences');
		let rowCount = table.rows.length-1;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
			{
				selectedSequences.push(sequences[i]);
			}
		}
		if (selectedSequences.length != 0) {
			let blob = new Blob([JSON.stringify(selectedSequences, undefined, 4)], {type: "application/json;charset=utf-8"});
			saveAs(blob, "sequences.json");
		} else {
			alert("No sequences selected")
		}
	} else if (editMode == EditMode.Settings) {
		let blob = new Blob([JSON.stringify(servoSettings, undefined, 4)], {type: "application/json;charset=utf-8"});
		saveAs(blob, "servoSettings.json");
	} else if (editMode == EditMode.Script) {
		let blob = new Blob([scripteditor.getValue()], {type: "text/plain;charset=utf-8"});
		saveAs(blob, "virtuoso.text");
	}
}

function selectAll(fld) {
	if (editMode == EditMode.Frames) {
		let table = $('frames');
		let rowCount = table.rows.length-1;
		let checkedCount = 0;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
				checkedCount++;
		}
		let checked = (checkedCount != rowCount);
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			sel.checked = checked;
		}
	} else if (editMode == EditMode.Settings) {
		let table = $('settings');
		let rowCount = table.rows.length-1;
		let checkedCount = 0;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#srvena");
			if (sel.checked)
				checkedCount++;
		}
		let checked = (checkedCount != rowCount);
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#srvena");
			sel.checked = checked;
		}
		updateServoSetting(0, 'srvena', fld.checked);
	} else if (editMode == EditMode.Sequences) {
		let table = $('sequences');
		let rowCount = table.rows.length-1;
		let checkedCount = 0;
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			if (sel.checked)
				checkedCount++;
		}
		let checked = (checkedCount != rowCount);
		for (let i = 0; i < rowCount; i++)
		{
			const row = table.rows[i+1];
			const sel = row.querySelector("#sel");
			sel.checked = checked;
		}
	}
	fld.checked = false;
}

function updateServoSetting(i,fld,val) {
	const searchParams = new URLSearchParams();
	searchParams.append('srv', (i-1 == -1) ? 'ALL' : i-1);
	searchParams.append(fld.substring(3), val);
	fetch('/api/changeSetting?'+searchParams)
		.catch(function(error) {
			console.log(error);
		});
}

function updateServoValue(evt) {
	let fld = evt.target;
	console.log('fld:',fld);
	const table = $('output');
	const rowCount = table.rows.length;
	let frame = frames[currentFrame];
	for (let i = 1; i < rowCount; i++) {
		const row = table.rows[i];
		const el = row.querySelector("#"+fld.id);
		if (el == fld) {
			let val = fld.value;
			if (fld.type != "checkbox") {
				if (typeof fld.value === "number") {
					fld.value = parseInt(fld.value);
				} else {
					fld.value = fld.value.substring(0, MAX_STRING_LENGTH);
				}
			} else {
				val = fld.checked;
			}
			if (fld.id == 'srvnam') {
				servoSettings[i-1][fld.id] = val;
			}
			if (fld.id == 'srvnam' || fld.id == 'srvpos') {
				updateServoSetting(i, fld.id, val);
			}
			if (fld.id != 'srvnam') {
				const srvidx = i.toString();
				if (fld.id == 'srvpos') {
					row.querySelector("#srvena").checked = true;
				}
				if (!frame.hasOwnProperty(srvidx)) {
					/* add empty */
					let servo = {...newServoFrame};
					servo.srvena = row.querySelector("#srvena").checked;
					servo.srvpos = row.querySelector("#srvpos").value;
					servo.srvdur = row.querySelector("#srvdur").value;
					servo.srveas = row.querySelector("#srveas").value;
					frame[srvidx] = servo;
				}
				let srvval = frame[srvidx];
				srvval[fld.id] = (fld.type == "checkbox") ? fld.checked :
					(typeof fld.value === "number") ? parseInt(fld.value) : fld.value;
				if (fld.id == 'srvpos') {
					srvval.srvena = true;
				}
			}
		}
	}
}

function updateFrameValue(evt) {
	let fld = evt.target;
	const table = $('frames');
	const rowCount = table.rows.length;
	for (let i = 1; i < rowCount; i++) {
		const row = table.rows[i];
		const el = row.querySelector("#"+fld.id);
		if (el == fld && fld.id != '#sel') {
			let frame = frames[i-1];
			frame[fld.id] = (typeof fld.value === "number") ? parseInt(fld.value) : fld.value;
		}
	}
}

function updateSequenceValue(evt) {
	let fld = evt.target;
	const table = $('sequences');
	const rowCount = table.rows.length;
	for (let i = 1; i < rowCount; i++) {
		const row = table.rows[i];
		const el = row.querySelector("#"+fld.id);
		if (el == fld && fld.id != '#sel') {
			let seq = sequences[i-1];
			seq[fld.id] = (typeof fld.value === "number") ? parseInt(fld.value) : fld.value;
		}
	}
}

function updateSettingsValue(evt) {
	let fld = evt.target;
	const table = $('settings');
	const rowCount = table.rows.length;
	for (let i = 1; i < rowCount; i++) {
		const row = table.rows[i];
		const el = row.querySelector("#"+fld.id);
		if (el == fld) {
			let val = fld.value;
			if (fld.type != "checkbox") {
				if (typeof fld.value === "number") {
					fld.value = parseInt(fld.value);
				} else {
					fld.value = fld.value.substring(0, MAX_STRING_LENGTH);
				}
			} else {
				val = fld.checked;
			}
			servoSettings[i-1][fld.id] = val;
			updateServoSetting(i, fld.id, val);
		}
	}
}

function showDropDown() {
	$("dropdown-content").classList.toggle("show");
}

window.onclick = function(event) {
	if (!event.target.matches('.dropbtn')) {
		let dropdowns = document.getElementsByClassName("dropdown-content");
		for (let i = 0; i < dropdowns.length; i++) {
			let openDropdown = dropdowns[i];
			if (openDropdown.classList.contains('show')) {
				openDropdown.classList.remove('show');
			}
		}
	}
}

function loadSettingsData(data) { 				
	servoSettings = data;
	savedSettings = encodeJSON('settings',servoSettings);
	loadSettings();
}
function addServoToFrame(index) {
	const srvidx = index.toString();
	let frame = frames[currentFrame];
	let servo = null;
	if (!frame.hasOwnProperty(srvidx)) {
		/* add empty */
		servo = {...newServoFrame};
		servo.srvena = false;
		frame[srvidx] = servo;
	} else {
		servo = frame[srvidx];
	}
	return servo;
}
function addEmptySequence() {
	sequences.push({
		// Field names are orded alphabetically for encoding (frames must be last)
		seqID: 	uuidv4(),
		seqNam: "Sequence #"+(sequences.length+1),
		seqframes: [{
			frdur: 50,
			frnam: "1",
			frbeg: "",
			frend: "",
		}]
	});
	selectSequenceAtIndex(sequences.length-1);
	for (let i = 0; i < servoSettings.length; i++) {
		let servo = addServoToFrame(i+1);
		servo.srvena = (servoSettings[i].srvena);
	}
}
function loadSequencesData(data) {
	let badCount = 1;
	sequences = new Array(savedSequenceOrderArray.length).fill(null);
	savedSequences = new Map();
	for (let i = 0; i < data.length; i++) {
		const seq = data[i];
		if (seq.hasOwnProperty('id') &&
				seq.hasOwnProperty('data'))
		{
			savedSequences.set(seq.id, seq.data);
			let utfstr = new Uint8Array(toUTF8Array(seq.data));
			let utfstr_mem = Module._malloc(utfstr.length + 1);

			Module.HEAPU8.set(utfstr, utfstr_mem);
			let result = Module._inflateJSON(0, utfstr_mem, utfstr.length);
			Module._free(utfstr_mem);
			let badDataData = false;
			if (result != null)
			{
				const decodedSeq = UTF8ToString(result);
				Module._free(result);
				try {
					let newSeq = JSON.parse(decodedSeq);
					let seqIndex = savedSequenceOrderArray.indexOf(newSeq.seqID);
					if (seqIndex == -1) {
						sequences.push(newSeq);
					} else {
						sequences[seqIndex] = newSeq;
					}
				} catch(e) {
					badDataData = true;
				}
			} else {
				badDataData = true;
			}
			if (badDataData) {
				alert("Failed to decode sequence: "+seq.id);
				let badSeq = {
					// Field names are orded alphabetically for encoding (frames must be last)
					seqID: 	seq.id,
					seqNam: "Corrupt Sequence #"+(badCount+1),
					seqframes: [{
						frdur: 50,
						frnam: "1",
						frbeg: "",
						frend: "",
					}]
				};
				let seqIndex = savedSequenceOrderArray.indexOf(badSeq.seqID);
				if (seqIndex == -1) {
					sequences.push(badSeq);
				} else {
					sequences[seqIndex] = badSeq;
				}
				badCount += 1;
			}
		}
		else
		{
			console.log("not decoding");
		}
	}
	sequences = sequences.filter(el => {
		return el !== null;
	});
	if (sequences.length == 0) {
		// Create new sequence
		addEmptySequence();
	}
	selectSequenceAtIndex(0);
	loadSequences();
	loadFrame();
	showAnimate();
}

function decodeScriptResponse(editor,base64)
{
	let utfstr = new Uint8Array(toUTF8Array(base64));
	let utfstr_mem = Module._malloc(utfstr.length + 1);

	Module.HEAPU8.set(utfstr, utfstr_mem);
	let result = Module._decodeText(utfstr_mem, utfstr.length);
	Module._free(utfstr_mem);

	if (result != null)
	{
		editor.setValue(UTF8ToString(result));
		editor.session.setUndoManager(new ace.UndoManager())
		Module._free(result);
	}
}
function decodeOrderResponse(base64)
{
	let utfstr = new Uint8Array(toUTF8Array(base64));
	let utfstr_mem = Module._malloc(utfstr.length + 1);

	Module.HEAPU8.set(utfstr, utfstr_mem);
	let result = Module._decodeText(utfstr_mem, utfstr.length);
	Module._free(utfstr_mem);

	let order = [];
	if (result != null)
	{
		let resultString = UTF8ToString(result);
		Module._free(result);
		try {
			order = JSON.parse(resultString);
			savedSequenceOrder = resultString;
		} catch(e) {
		}
	}
	return order;
}

if (window.location.protocol == "file:") {
	alert("Not supported.");
} else {
	loadFrame();
	loadSettings();
	showAnimate();
}

function orderedStringify(node)
{
	let seen = [];
	function stringify(node, cmp) {
		if (node && node.toJSON && typeof node.toJSON === 'function') {
				node = node.toJSON();
		}

		if (node === undefined) return;
		if (typeof node == 'number') return isFinite(node) ? '' + node : 'null';
		if (typeof node !== 'object') return JSON.stringify(node);

		let i, out;
		if (Array.isArray(node)) {
				out = '[';
				for (i = 0; i < node.length; i++) {
						if (i) out += ',';
						out += stringify(node[i]) || 'null';
				}
				return out + ']';
		}
		if (node === null) return 'null';
		if (seen.indexOf(node) !== -1) {
				throw new TypeError('Not supported');
		}
		let seenIndex = seen.push(node) - 1;
		let keys = Object.keys(node).sort(function (a, b) {
			if (isNaN(a)) {
				if (isNaN(b)) {
					return a > b ? 1 : -1;
				} else {
					return -1;
				}
			} else {
				if (!isNaN(b)) {
					return parseInt(a) > parseInt(b) ? 1 : -1;
				} else {
					return -1;
				}
			}
		});
		out = '';
		for (i = 0; i < keys.length; i++) {
				let key = keys[i];
				let value = stringify(node[key]);

				if (!value) continue;
				if (out) out += ',';
				out += JSON.stringify(key) + ':' + value;
		}
		seen.splice(seenIndex, 1);
		return '{' + out + '}';
	}
	return stringify(node);
}

async function postSequence(id, encodedSequence) {
	const searchParams = new URLSearchParams();
	searchParams.append('id', id);
	let url = '/api/upload/sequence'+'?'+searchParams;
	const response = await fetch(url, {
			method: 'POST',
			headers: {
				'Content-Type': 'application/base64'
			},
			body: encodedSequence
		});
	return response;
}

async function postSettings(text = '') {
	const response = await fetch('/api/upload/settings', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/base64'
			},
			body: text
		});
	return response;
}

async function postScript(text = '') {
	const response = await fetch('/api/upload/script', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/base64'
			},
			body: text
		});
	return response;
}

async function postOrder(text = '') {
	const response = await fetch('/api/upload/order', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/base64'
			},
			body: text
		});
	return response;
}

async function postVirtuoso(text = '') {
	const response = await fetch('/api/upload/virtuoso', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/base64'
			},
			body: text
		});
	return response;
}

var Virtuoso = {
	print: (function() {
		return function(text) {
			console.log(text);
		};
	})(),
	printErr: function(text) {
		let PREFIXout = "OUT:";
		let PREFIXscript = "ERROR:script:";
		if (text.startsWith(PREFIXout))
		{
			text = text.slice(PREFIXout.length);
			console.log(text);
		}
		else if (text.startsWith(PREFIXscript))
		{
			text = text.slice(PREFIXscript.length);
			const errormsg = text.split(":");
			if (errormsg.length == 3)
			{
				let rowNumber = parseInt(errormsg[0]);
				let colNumber = parseInt(errormsg[1]);
				let msg = errormsg[2].trim();
				console.log("ROW:",rowNumber);
				console.log("COL:",colNumber);
				console.log("MSG:",msg);
				scripteditor.getSession().setAnnotations([{
					row: rowNumber,
					column: colNumber,
					text: msg,
					type: "error" // also warning and information
				}]);
				alert(errormsg[2]);
			}
			else
			{
				alert(text);
			}
		}
		else
		{
			let PREFIX = "ERROR:";
			if (text.startsWith(PREFIX))
			{
				text = text.slice(PREFIX.length);
				alert(text);
			}
		}
	}
};

function getScriptSubroutine(frame, routineMap, lowestVal, highestVal) {
	let routineName = "frame_"+lowestVal+".."+highestVal;
	if (!routineMap.has(routineName))
	{
		let routine = "sub "+routineName+"\n";
		for (i = highestVal; i >= lowestVal; i--)
		{
			routine += "  "+i+" servo\n";
		}
		routine += "  delay\n";
		routine += "  return\n";
		routineMap.set(routineName, routine);
	}
	return routineName;
}

function syncScript() {
	let text = "\n";
	text += "### Sequence subroutines: ###\n";
	text += "\n";
	for (let qi = 0; qi < sequences.length; qi++)
	{
		text += "# Sequence "+qi+"\n";
		text += "sub Sequence_"+qi+"\n";
		for (let fi = 0; fi < frames.length; fi++)
		{
			let lowestVal = 0xFFFF;
			let highestVal = 0
			let frame = frames[fi];
			if (frame.hasOwnProperty("frbeg")) {
				let serialBegin = frame.frbeg;
				if (serialBegin.length > 0)
				{
					text += "  ";
					for (a = 0; a < serialBegin.length; a++) {
						let ch = (serialBegin.charCodeAt(a) & 0xFF);
						text += '0x'+serialBegin.charCodeAt(a).toString(16)+' ';
					}
					text += " 0x00 serial_send\n";
				}
			}
			for (let si = 0; si < servoSettings.length; si++)
			{
				let srvidx = (si+1).toString();
				if (frame.hasOwnProperty(srvidx)) {
					let srvval = frame[srvidx];
					if (srvval.srvena) {
						text += "  "+srvval.srveas+" "+srvval.srvdur+" "+srvval.srvpos+" "+si+" servox\n";
					}
				}
			}
			let duration = frame.frdur * 10;
			if (duration == 0)
			{
				text += "  begin get_moving_state while repeat\n";
			}
			else
			{
				text += "  "+duration+" delay\n";
			}
			if (frame.hasOwnProperty("frend")) {
				let serialEnd = frame.frend;
				console.log(serialEnd);
				if (serialEnd.length > 0)
				{
					text += "  ";
					for (a = 0; a < serialEnd.length; a++) {
						text += '0x'+serialEnd.charCodeAt(a).toString(16)+' ';
					}
					text += "0x00 serial_send\n";
				}
			}
		}
		text += "  return\n";
		text += "\n";
	}
	//console.log(text);
	scripteditor.setValue(text);
	scripteditor.getSession().setAnnotations([{}]);
	scripteditor.session.setUndoManager(new ace.UndoManager())
}

function saveSettings() {
	let result = encodeJSON('settings',servoSettings);
	if (result != null && result != savedSettings) {
		postSettings(result)
			.then((response) =>  {
				if (!response.ok)
				{
						alert("Failed to upload settings.");
				}
				else
				{
					savedSettings = result;
				}
			})
			.catch((error) => {
				alert("Failed to upload settings.");
			});
	}
}

function upload_button() {
	if (editMode == EditMode.Frames) {
		saveSequence(activeSequence, false, "Failed to upload sequence.");
	} if (editMode == EditMode.Sequences) {
		saveSequences();
	} else if (editMode == EditMode.Settings) {
		saveSettings();
	} else if (editMode == EditMode.Script) {
		uploadScript();
	}
}

function uploadScript() {
	let text = scripteditor.getValue();
	scripteditor.getSession().setAnnotations([{}]);

	let utfstr = new Uint8Array(toUTF8Array(text));
	let utfstr_mem = Module._malloc(utfstr.length + 1);

	Module.HEAPU8.set(utfstr, utfstr_mem);

	let result = Module._encodeText(utfstr_mem, utfstr.length);
	Module._free(utfstr_mem);
	if (result != null)
	{
		let scriptBytes = UTF8ToString(result);
		Module._free(result);
		postScript(scriptBytes)
		.then((response) =>  {
			if (response.ok)
			{
				let utfstr = new Uint8Array(toUTF8Array(text));
				let utfstr_mem = Module._malloc(utfstr.length + 1);

				Module.HEAPU8.set(utfstr, utfstr_mem);

				let result = Module._compile(utfstr_mem, utfstr.length);
				Module._free(utfstr_mem);
				if (result != null)
				{
					let virtuosoBytes = UTF8ToString(result);
					Module._free(result);
					postVirtuoso(virtuosoBytes)
					.then((response) =>  {
						if (response.ok)
						{
								alert("Uploaded");
						}
					})
					.catch((error) => {
						console.log('Failed to upload script:', error);
					});
				}
			}
		})
		.catch((error) => {
			console.log('Failed to upload script:', error);
		});
	}
}

function uploadSequenceOrder() {
	let order = [];
	for (let i = 0; i < sequences.length; i++) {
		order.push(sequences[i].seqID);
	}
	let orderString = JSON.stringify(order);
	if (savedSequenceOrder != orderString) {
		let utfstr = new Uint8Array(toUTF8Array(orderString));
		let utfstr_mem = Module._malloc(utfstr.length + 1);

		Module.HEAPU8.set(utfstr, utfstr_mem);

		let result = Module._encodeText(utfstr_mem, utfstr.length);
		Module._free(utfstr_mem);
		if (result != null)
		{
			let orderBytes = UTF8ToString(result);
			Module._free(result);
			postOrder(orderBytes)
			.then((response) =>  {
				if (!response.ok) {
					console.log('Failed to upload sequence order');
				} else {
					savedSequenceOrder = orderString;
				}
			})
			.catch((error) => {
				console.log('Failed to upload sequence order:', error);
			});
		}
	}
}

function toUTF8Array(str) {
	let utf8 = [];
	for (let i=0; i < str.length; i++) {
		let charcode = str.charCodeAt(i);
		if (charcode < 0x80) {
			utf8.push(charcode);
		} else if (charcode < 0x800) {
			utf8.push(0xc0 | (charcode >> 6), 0x80 | (charcode & 0x3f));
		} else if (charcode < 0xd800 || charcode >= 0xe000) {
			utf8.push(0xe0 | (charcode >> 12), 0x80 | ((charcode>>6) & 0x3f), 0x80 | (charcode & 0x3f));
		} else {
			i++;
			charcode = 0x10000 + (((charcode & 0x3ff)<<10) | (str.charCodeAt(i) & 0x3ff));
			utf8.push(0xf0 | (charcode >>18), 0x80 | ((charcode>>12) & 0x3f), 0x80 | ((charcode>>6) & 0x3f),
				0x80 | (charcode & 0x3f));
		}
	}
	return utf8;
}

function loadAll(data) {
	if (data.hasOwnProperty('order')) {
		savedSequenceOrderArray = decodeOrderResponse(data.order);
	}
	loadSettingsData(data.settings);
	loadSequencesData(data.seq);
	if (data.hasOwnProperty('virtuoso')) {
		decodeScriptResponse(scripteditor, data.virtuoso)
	}
}

$('add_button').onclick=add_button;
$('edit_button').onclick=edit_button;
$('up_button').onclick=up_button;
$('down_button').onclick=down_button;
$('del_button').onclick=delete_button;
$('playSeq').onclick=play_button;
$('stopSeq').onclick=stop_button;
$('prevFrame').onclick=prevFrame;
$('nextFrame').onclick=nextFrame;
$('sync_script').onclick=syncScript;
$('upload_button').onclick=upload_button;
$('navbarinfo').onclick=goto_frame;
$('dropbtn').onclick=showDropDown;
$('showAnimate').onclick=showAnimate;
$('showTimeline').onclick=showTimeline;
$('showFrames').onclick=showFrames;
$('showSequences').onclick=showSequences;
$('showSettings').onclick=showSettings;
$('showScript').onclick=showScript;
$('saveToFile').onclick=saveToJSONFile;
$('openFile').onclick=function() { openFile(loadFromFile); };

$('col0').onchange=updateServoValue;
$('col1').onchange=updateServoValue;
$('col2').onchange=updateServoValue;
$('col3').onchange=updateServoValue;
$('col4').onchange=updateServoValue;

$('frselall').onchange=selectAll;
$('frcol1').onchange=updateFrameValue;
$('frcol2').onchange=updateFrameValue;
$('frcol3').onchange=updateFrameValue;
$('frcol4').onchange=updateFrameValue;

$('seqselall').onchange=selectAll;
$('seqcol1').onchange=updateSequenceValue;

$('setselall').onchange=selectAll;
$('setcol0').onchange=updateSettingsValue;
$('setcol1').onchange=updateSettingsValue;
$('setcol2').onchange=updateSettingsValue;
$('setcol3').onchange=updateSettingsValue;
$('setcol4').onchange=updateSettingsValue;
$('setcol5').onchange=updateSettingsValue;

const nam0_min = getStyleRuleWidth('.nam0_css');
const pos0_min = getStyleRuleWidth('.pos0_css');
const eas0_min = getStyleRuleWidth('.eas0_css');
const seqnam_min = getStyleRuleWidth('.seqnam0_css');
const frnam_min = getStyleRuleWidth('.setnam0_css');
const serial_min = getStyleRuleWidth('.serial0_css');
function adjustWindowSize() {
	let clientWidth = document.documentElement.clientWidth/1.2;
	let nam0_width = Math.max(clientWidth/6, nam0_min);
	let serial_width = Math.max((clientWidth/100)*20, serial_min);
	let seqnam_width = Math.max((clientWidth/100)*60, seqnam_min);
	let frnam_width = Math.max((clientWidth/100)*20, frnam_min);
	let eas0_width = Math.min(Math.max(clientWidth/6, eas0_min), 100);
	let pos0_width = (clientWidth - nam0_width - eas0_width)/1.3;
	modifyStyleRuleWidth('.nam0_css', nam0_width);
	modifyStyleRuleWidth('.pos0_css', pos0_width);
	modifyStyleRuleWidth('.eas0_css', eas0_width);
	modifyStyleRuleWidth('.seqnam0_css', seqnam_width);
	modifyStyleRuleWidth('.setnam0_css', frnam_width);
	modifyStyleRuleWidth('.serial0_css', serial_width);
	modifyStyleRuleWidth('.navbar', document.documentElement.clientWidth);
}
adjustWindowSize();
window.onresize = adjustWindowSize;

scripteditor = ace.edit("script_editor");
scripteditor.setOptions({
	//showGutter: false
});

////////////////////////////////////////

function UndoState(state, description) {
	// this.state = JSON.stringify(state);
	this.state = state.getJSONString();
	this.description = description;
}

function UndoManager(dispatcher, max) {
	this.dispatcher = dispatcher;
	this.MAX_ITEMS = max || 100;
	this.clear();
}

UndoManager.prototype.save = function(state, suppress) {
	var states = this.states;
	var next_index = this.index + 1;
	var to_remove = states.length - next_index;
	states.splice(next_index, to_remove, state);

	if (states.length > this.MAX_ITEMS) {
		states.shift();
	}

	this.index = states.length - 1;

	// console.log('Undo State Saved: ', state.description);
	if (!suppress) this.dispatcher.fire('state:save', state.description);
};

UndoManager.prototype.clear = function() {
	this.states = [];
	this.index = -1;
	// FIXME: leave default state or always leave one state?
};

UndoManager.prototype.canUndo = function() {
	return this.index > 0;
	// && this.states.length > 1
};

UndoManager.prototype.canRedo = function() {
	return this.index < this.states.length - 1;
};

UndoManager.prototype.undo = function() {
	if (this.canUndo()) {
		this.dispatcher.fire('status', 'Undo: ' + this.get().description);
		this.index--;
	} else {
		this.dispatcher.fire('status', 'Nothing to undo');
	}

	return this.get();
};

UndoManager.prototype.redo = function() {
	if (this.canRedo()) {
		this.index++;
		this.dispatcher.fire('status', 'Redo: ' + this.get().description);
	} else {
		this.dispatcher.fire('status', 'Nothing to redo');
	}

	return this.get();
};

UndoManager.prototype.get = function() {
	return this.states[this.index];
};

/**************************/
// Dispatcher
/**************************/

function Dispatcher() {

	var event_listeners = {

	};

	function on(type, listener) {
		if (!(type in event_listeners)) {
			event_listeners[type] = [];
		}
		var listeners = event_listeners[type];
		listeners.push(listener);
	}

	function fire(type) {
		var args = Array.prototype.slice.call(arguments);
		args.shift();
		var listeners = event_listeners[type];
		if (!listeners) return;
		for (var i = 0; i < listeners.length; i++) {
			var listener = listeners[i];
			listener.apply(listener, args);
		}
	}

	this.on = on;
	this.fire = fire;

}

const Theme = {
	// photoshop colors
	a: '#343434',
	b: '#535353',
	c: '#b8b8b8',
	d: '#d6d6d6',
};

var DEFAULT_TIME_SCALE = 60;

// Dimensions
var LayoutConstants = {
	LINE_HEIGHT: 26,
	DIAMOND_SIZE: 10,
	MARKER_TRACK_HEIGHT: 60,
	width: 600,
	height: 400,
	TIMELINE_SCROLL_HEIGHT: 0,
	LEFT_PANE_WIDTH: 250,
	time_scale: DEFAULT_TIME_SCALE, // number of pixels to 1 second
	default_length: 20, // seconds
	DEFAULT_TIME_SCALE
};

/**************************/
// Tweens
/**************************/

var Tweens = {
	none: function(k) {
		return 0;
	},
	linear: function(k) {
		return k;
	},
	quadEaseIn: function(k) {
		return k * k;
	},
	quadEaseOut: function(k) {
		return - k * ( k - 2 );
	},
	quadEaseInOut: function(k) {
		if ( ( k *= 2 ) < 1 ) return 0.5 * k * k;
		return - 0.5 * ( --k * ( k - 2 ) - 1 );
	}
};

var STORAGE_PREFIX$2 = 'timeliner-';

/**************************/
// Utils
/**************************/

function firstDefined$1() {
	for (var i = 0; i < arguments.length; i++) {
		if (typeof arguments[i] !== 'undefined') {
			return arguments[i];
		}
	}
	return undefined;
}

function style$4(element, ...styles) {
	for (var i = 0; i < styles.length; ++i) {
		var style = styles[i];
		for (var s in style) {
			element.style[s] = style[s];
		}
	}
}

function saveToFile$1(string, filename) {
	var a = document.createElement("a");
	document.body.appendChild(a);
	a.style = "display: none";

	var blob = new Blob([string], { type: 'octet/stream' }), // application/json
		url = window.URL.createObjectURL(blob);

	a.href = url;
	a.download = filename;

	fakeClick(a);

	setTimeout(function() {
		// cleanup and revoke
		window.URL.revokeObjectURL(url);
		document.body.removeChild(a);
	}, 500);
}



var input, openCallback;

function handleFileSelect(evt) {
	var files = evt.target.files; // FileList object

	console.log('handle file select', files.length);

	var f = files[0];
	if (!f) return;
	// Can try to do MINE match
	// if (!f.type.match('application/json')) {
	//   return;
	// }
	console.log('match', f.type);

	var reader = new FileReader();

	// Closure to capture the file information.
	reader.onload = function(e) {
		var data = e.target.result;
		openCallback(data);
	};

	reader.readAsText(f);

	input.value = '';
}


function openAs$1(callback, target) {
	console.log('openfile...');
	openCallback = callback;

	if (!input) {
		input = document.createElement('input');
		input.style.display = 'none';
		input.type = 'file';
		input.addEventListener('change', handleFileSelect);
		target = target || document.body;
		target.appendChild(input);
	}

	fakeClick(input);
}

function fakeClick(target) {
	var e = document.createEvent("MouseEvents");
	e.initMouseEvent(
		'click', true, false, window, 0, 0, 0, 0, 0,
		false, false, false, false, 0, null
	);
	target.dispatchEvent(e);
}

function format_friendly_seconds(s, type) {
	// TODO Refactor to 60fps???
	// 20 mins * 60 sec = 1080
	// 1080s * 60fps = 1080 * 60 < Number.MAX_SAFE_INTEGER

	var raw_secs = s | 0;
	var secs = raw_secs % 60;
	var raw_mins = raw_secs / 60 | 0;
	var mins = raw_mins % 60;

	var secs_str = (secs / 100).toFixed(2).substring(2);

	var str = mins + ':' + secs_str;

	if (s % 1 > 0) {
		var t2 = (s % 1) * 60;
		if (type === 'frames') str = secs + '+' + t2.toFixed(0) + 'f';
		else str += ((s % 1).toFixed(2)).substring(1);
		// else str = mins + ':' + secs_micro;
		// else str = secs_micro + 's'; /// .toFixed(2)
	}
	return str;
}

// get object at time
function findTimeinLayer(layer, time) {
	var values = layer.values;
	var i, il;

	// TODO optimize by checking time / binary search

	for (i=0, il=values.length; i<il; i++) {
		var value = values[i];
		if (value.time === time) {
			return {
				index: i,
				object: value
			};
		} else if (value.time > time) {
			return i;
		}
	}

	return i;
}


function timeAtLayer(layer, t) {
	// Find the value of layer at t seconds.
	// this expect layer to be sorted
	// not the most optimized for now, but would do.

	var values = layer.values;
	var i, il, entry, prev_entry;

	il = values.length;

	// can't do anything
	if (il === 0) return;

	if (layer._mute) return

	// find boundary cases
	entry = values[0];
	if (t < entry.time) {
		return {
			value: entry.value,
			can_tween: false, // cannot tween
			keyframe: false // not on keyframe
		};
	}

	for (i=0; i<il; i++) {
		prev_entry = entry;
		entry = values[i];

		if (t === entry.time) {
			// only exception is on the last KF, where we display tween from prev entry
			if (i === il - 1) {
				return {
					// index: i,
					entry: prev_entry,
					tween: prev_entry.tween,
					can_tween: il > 1,
					value: entry.value,
					keyframe: true
				};
			}
			return {
				// index: i,
				entry: entry,
				tween: entry.tween,
				can_tween: il > 1,
				value: entry.value,
				keyframe: true // il > 1
			};
		}
		if (t < entry.time) {
			// possibly a tween
			if (!prev_entry.tween) { // or if value is none
				return {
					value: prev_entry.value,
					tween: false,
					entry: prev_entry,
					can_tween: true,
					keyframe: false
				};
			}

			// calculate tween
			var time_diff = entry.time - prev_entry.time;
			var value_diff = entry.value - prev_entry.value;
			var tween = prev_entry.tween;

			var dt = t - prev_entry.time;
			var k = dt / time_diff;
			var new_value = prev_entry.value + Tweens[tween](k) * value_diff;

			return {
				entry: prev_entry,
				value: new_value,
				tween: prev_entry.tween,
				can_tween: true,
				keyframe: false
			};
		}
	}
	// time is after all entries
	return {
		value: entry.value,
		can_tween: false,
		keyframe: false
	};

}


function proxy_ctx$1(ctx) {
	// Creates a proxy 2d context wrapper which
	// allows the fluent / chaining API.
	var wrapper = {};

	function proxy_function(c) {
		return function() {
			// Warning: this doesn't return value of function call
			ctx[c].apply(ctx, arguments);
			return wrapper;
		};
	}

	function proxy_property(c) {
		return function(v) {
			ctx[c] = v;
			return wrapper;
		};
	}

	wrapper.run = function(args) {
		args(wrapper);
		return wrapper;
	};

	for (var c in ctx) {
		// if (!ctx.hasOwnProperty(c)) continue;
		// console.log(c, typeof(ctx[c]), ctx.hasOwnProperty(c));
		// string, number, boolean, function, object

		var type = typeof(ctx[c]);
		switch (type) {
		case 'object':
			break;
		case 'function':
			wrapper[c] = proxy_function(c);
			break;
		default:
			wrapper[c] = proxy_property(c);
			break;
		}
	}

	return wrapper;
}

var utils = {
	STORAGE_PREFIX: STORAGE_PREFIX$2,
	firstDefined: firstDefined$1,
	style: style$4,
	saveToFile: saveToFile$1,
	openAs: openAs$1,
	format_friendly_seconds,
	findTimeinLayer,
	timeAtLayer,
	proxy_ctx: proxy_ctx$1
};

/* Over simplistic Event Dispatcher */

class Do {
	constructor() {
		this.listeners = new Set();
	}

	do(callback) {
		this.listeners.add(callback);
	}

	undo(callback) {
		this.listeners.delete(callback);
	}

	fire(...args) {
		for (let l of this.listeners) {
			l(...args);
		}
	}
}

function handleDrag(element, ondown, onmove, onup, down_criteria) {
	var pointer = null;
	var bounds = element.getBoundingClientRect();

	element.addEventListener('mousedown', onMouseDown);

	function onMouseDown(e) {
		handleStart(e);

		if (down_criteria && !down_criteria(pointer)) {
			pointer = null;
			return;
		}


		document.addEventListener('mousemove', onMouseMove);
		document.addEventListener('mouseup', onMouseUp);

		ondown(pointer);

		e.preventDefault();
	}

	function onMouseMove(e) {
		handleMove(e);
		onmove(pointer);
	}

	function handleStart(e) {
		bounds = element.getBoundingClientRect();
		var currentx = e.clientX, currenty = e.clientY;
		pointer = {
			startx: currentx,
			starty: currenty,
			x: currentx,
			y: currenty,
			dx: 0,
			dy: 0,
			offsetx: currentx - bounds.left,
			offsety: currenty - bounds.top,
			moved: false
		};
	}

	function handleMove(e) {
		bounds = element.getBoundingClientRect();
		var currentx = e.clientX,
			currenty = e.clientY,
			offsetx = currentx - bounds.left,
			offsety = currenty - bounds.top;
		pointer.x = currentx;
		pointer.y = currenty;
		pointer.dx = e.clientX - pointer.startx;
		pointer.dy = e.clientY - pointer.starty;
		pointer.offsetx = offsetx;
		pointer.offsety = offsety;

		// If the pointer dx/dy is _ever_ non-zero, then it's moved
		pointer.moved = pointer.moved || pointer.dx !== 0 || pointer.dy !== 0;
	}

	function onMouseUp(e) {
		handleMove(e);
		onup(pointer);
		pointer = null;

		document.removeEventListener('mousemove', onMouseMove);
		document.removeEventListener('mouseup', onMouseUp);
	}

	element.addEventListener('touchstart', onTouchStart);

	function onTouchStart(te) {

		if (te.touches.length == 1) {

			var e = te.touches[0];
			if (down_criteria && !down_criteria(e)) return;
			te.preventDefault();
			handleStart(e);
			ondown(pointer);
		}

		element.addEventListener('touchmove', onTouchMove);
		element.addEventListener('touchend', onTouchEnd);
	}

	function onTouchMove(te) {
		var e = te.touches[0];
		onMouseMove(e);
	}

	function onTouchEnd(e) {
		// var e = e.touches[0];
		onMouseUp(e);
		element.removeEventListener('touchmove', onTouchMove);
		element.removeEventListener('touchend', onTouchEnd);
	}


	// this.release = function() {
	// 	element.removeEventListener('mousedown', onMouseDown);
	// 	element.removeEventListener('touchstart', onTouchStart);
	// };
}

const { firstDefined, style: style$3 } = utils;

/**************************/
// UINumber
/**************************/

function UINumber(config) {
	config = config || {};
	var min = config.min === undefined ? -Infinity : config.min;

	// config.xstep and config.ystep allow configuring adjustment
	// speed across each axis.
	// config.wheelStep and config.wheelStepFine allow configuring
	// adjustment speed for mousewheel, and mousewheel while holding <alt>

	// If only config.step is specified, all other adjustment speeds
	// are set to the same value.
	var xstep = firstDefined(config.xstep, config.step, 0.001);
	var ystep = firstDefined(config.ystep, config.step, 0.1);
	var wheelStep = firstDefined(config.wheelStep, ystep);
	var wheelStepFine = firstDefined(config.wheelStepFine, xstep);

	var precision = config.precision || 3;
	// Range
	// Max

	var span = document.createElement('input');
	// span.type = 'number'; // spinner

	style$3(span, {
		textAlign: 'center',
		fontSize: '10px',
		padding: '1px',
		cursor: 'ns-resize',
		width: '40px',
		margin: 0,
		marginRight: '10px',
		appearance: 'none',
		outline: 'none',
		border: 0,
		background: 'none',
		borderBottom: '1px dotted '+ Theme.c,
		color: Theme.c
	});

	var me = this;
	var value = 0, unchanged_value;

	this.onChange = new Do();

	span.addEventListener('change', function(e) {
		console.log('input changed', span.value);
		value = parseFloat(span.value, 10);

		fireChange();
	});

	// Allow keydown presses in inputs, don't allow parent to block them
	span.addEventListener('keydown', function(e) {
		e.stopPropagation();
	});

	span.addEventListener('focus', function(e) {
		span.setSelectionRange(0, span.value.length);
	});

	span.addEventListener('wheel', function(e) {
		// Disregard pixel/line/page scrolling and just
		// use event direction.
		var inc = e.deltaY > 0? 1 : -1;
		if (e.altKey) {
			inc *= wheelStepFine;
		} else {
			inc *= wheelStep;
		}
		value = clamp(value + inc);
		fireChange();
	});

	handleDrag(span, onDown, onMove, onUp);

	function clamp(value) {
		return Math.max(min, value);
	}

	function onUp(e) {
		if (e.moved) fireChange();
		else {
			// single click
			span.focus();
		}
	}

	function onMove(e) {
		var dx = e.dx;
		var dy = e.dy;

		value = unchanged_value + (dx * xstep) + (dy * -ystep);

		value = clamp(value);

		// value = +value.toFixed(precision); // or toFixed toPrecision
		me.onChange.fire(value, true);
	}

	function onDown(e) {
		unchanged_value = value;
	}

	function fireChange() {
		me.onChange.fire(value);
	}

	this.dom = span;

	// public
	this.setValue = function(v) {
		value = v;
		span.value = value.toFixed(precision);
	};

	this.paint = function() {
		if (value && document.activeElement !== span) {
			span.value = value.toFixed(precision);
		}
	};
}

// TODO - tagged by index instead, work off layers.

function LayerView(layer, dispatcher) {
	var dom = document.createElement('div');

	var label = document.createElement('span');

	label.style.cssText = 'font-size: 12px; padding: 4px;';

	label.addEventListener('click', function(e) {
		// context.dispatcher.fire('label', channelName);
	});

	label.addEventListener('mouseover', function(e) {
		// context.dispatcher.fire('label', channelName);
	});

	var dropdown = document.createElement('select');
	var option;
	dropdown.style.cssText = 'font-size: 10px; width: 60px; margin: 0; float: right; text-align: right;';

	for (var k in Tweens) {
		option = document.createElement('option');
		option.text = k;
		dropdown.appendChild(option);
	}

	dropdown.addEventListener('change', function(e) {
		dispatcher.fire('ease', layer, dropdown.value);
	});
	var height = (LayoutConstants.LINE_HEIGHT - 1);

	var keyframe_button = document.createElement('button');
	keyframe_button.innerHTML = '&#9672;'; // '&diams;' &#9671; 9679 9670 9672
	keyframe_button.style.cssText = 'background: none; font-size: 12px; padding: 0px; font-family: monospace; float: right; width: 20px; height: ' + height + 'px; border-style:none; outline: none;'; //  border-style:inset;

	keyframe_button.addEventListener('click', function(e) {
		console.log('clicked:keyframing...', state.get('_value').value);
		dispatcher.fire('keyframe', layer, state.get('_value').value);
	});

	/*
	// Prev Keyframe
	var button = document.createElement('button');
	button.textContent = '<';
	button.style.cssText = 'font-size: 12px; padding: 1px; ';
	dom.appendChild(button);

	// Next Keyframe
	button = document.createElement('button');
	button.textContent = '>';
	button.style.cssText = 'font-size: 12px; padding: 1px; ';
	dom.appendChild(button);


	*/

	// function ToggleButton(text) {
	// 	// for css based button see http://codepen.io/mallendeo/pen/eLIiG

	// 	var button = document.createElement('button');
	// 	button.textContent = text;

	// 	utils.style(button, {
	// 		fontSize: '12px',
	// 		padding: '1px',
	// 		borderSize: '2px',
	// 		outline: 'none',
	// 		background: Theme.a,
	// 		color: Theme.c,
	// 	});

	// 	this.pressed = false;

	// 	button.onclick = () => {
	// 		this.pressed = !this.pressed;

	// 		utils.style(button, {
	// 			borderStyle: this.pressed ? 'inset' : 'outset', // inset outset groove ridge
	// 		});

	// 		if (this.onClick) this.onClick();
	// 	};

	// 	this.dom = button;

	// }

	// // Solo
	// var solo_toggle = new ToggleButton('S');
	// dom.appendChild(solo_toggle.dom);

	// solo_toggle.onClick = function() {
	// 	dispatcher.fire('action:solo', layer, solo_toggle.pressed);
	// };

	// // Mute
	// var mute_toggle = new ToggleButton('M');
	// dom.appendChild(mute_toggle.dom);

	// mute_toggle.onClick = function() {
	// 	dispatcher.fire('action:mute', layer, mute_toggle.pressed);
	// };

	// var number = new UINumber(layer);

	// number.onChange.do(function(value, done) {
	// 	state.get('_value').value = value;
	// 	dispatcher.fire('value.change', layer, value, done);
	// });

	// utils.style(number.dom, {
	// 	float: 'right'
	// });

	dom.appendChild(label);
	// dom.appendChild(keyframe_button);
	// dom.appendChild(number.dom);
	// dom.appendChild(dropdown);

	utils.style(dom, {
		textAlign: 'left',
		margin: '0px 0px 0px 5px',
		borderBottom: '1px solid ' + Theme.b,
		top: 0,
		left: 0,
		height: (LayoutConstants.LINE_HEIGHT - 1 ) + 'px',
		color: Theme.c
	});

	this.dom = dom;

	this.repaint = repaint;
	var state;

	this.setState = function(l, s) {
		layer = l;
		state = s;

		var tmp_value = state.get('_value');
		if (tmp_value.value === undefined) {
			tmp_value.value = 0;
		}

		// number.setValue(tmp_value.value);
		label.textContent = state.get('name').value;

		repaint();
	};

	function repaint(s) {

		dropdown.style.opacity = 0;
		dropdown.disabled = true;
		keyframe_button.style.color = Theme.b;
		var o = utils.timeAtLayer(layer, s);

		if (!o) return;

		if (o.can_tween) {
			dropdown.style.opacity = 1;
			dropdown.disabled = false;
			// if (o.tween)
			dropdown.value = o.tween ? o.tween : 'none';
			if (dropdown.value === 'none') dropdown.style.opacity = 0.5;
		}

		if (o.keyframe) {
			keyframe_button.style.color = Theme.c;
			// keyframe_button.disabled = true;
			// keyframe_button.style.borderStyle = 'inset';
		}

		state.get('_value').value = o.value;
		// number.setValue(o.value);
		// number.paint();

		dispatcher.fire('target.notify', layer.name, o.value);
	}

}

const font = {
	"unitsPerEm": 1792,
	"ascender": 1536,
	"descender": -256,
	"fonts": {
		"plus": {
			"advanceWidth": 1408,
			"commands": "M,1408,800 C,1408,853,1365,896,1312,896 L,896,896 L,896,1312 C,896,1365,853,1408,800,1408 L,608,1408 C,555,1408,512,1365,512,1312 L,512,896 L,96,896 C,43,896,0,853,0,800 L,0,608 C,0,555,43,512,96,512 L,512,512 L,512,96 C,512,43,555,0,608,0 L,800,0 C,853,0,896,43,896,96 L,896,512 L,1312,512 C,1365,512,1408,555,1408,608 Z"
		},
		"minus": {
			"advanceWidth": 1408,
			"commands": "M,1408,800 C,1408,853,1365,896,1312,896 L,96,896 C,43,896,0,853,0,800 L,0,608 C,0,555,43,512,96,512 L,1312,512 C,1365,512,1408,555,1408,608 Z"
		},
		"ok": {
			"advanceWidth": 1792,
			"commands": "M,1671,970 C,1671,995,1661,1020,1643,1038 L,1507,1174 C,1489,1192,1464,1202,1439,1202 C,1414,1202,1389,1192,1371,1174 L,715,517 L,421,812 C,403,830,378,840,353,840 C,328,840,303,830,285,812 L,149,676 C,131,658,121,633,121,608 C,121,583,131,558,149,540 L,511,178 L,647,42 C,665,24,690,14,715,14 C,740,14,765,24,783,42 L,919,178 L,1643,902 C,1661,920,1671,945,1671,970 Z"
		},
		"remove": {
			"advanceWidth": 1408,
			"commands": "M,1298,214 C,1298,239,1288,264,1270,282 L,976,576 L,1270,870 C,1288,888,1298,913,1298,938 C,1298,963,1288,988,1270,1006 L,1134,1142 C,1116,1160,1091,1170,1066,1170 C,1041,1170,1016,1160,998,1142 L,704,848 L,410,1142 C,392,1160,367,1170,342,1170 C,317,1170,292,1160,274,1142 L,138,1006 C,120,988,110,963,110,938 C,110,913,120,888,138,870 L,432,576 L,138,282 C,120,264,110,239,110,214 C,110,189,120,164,138,146 L,274,10 C,292,-8,317,-18,342,-18 C,367,-18,392,-8,410,10 L,704,304 L,998,10 C,1016,-8,1041,-18,1066,-18 C,1091,-18,1116,-8,1134,10 L,1270,146 C,1288,164,1298,189,1298,214 Z"
		},
		"zoom_in": {
			"advanceWidth": 1664,
			"commands": "M,1024,736 C,1024,753,1009,768,992,768 L,768,768 L,768,992 C,768,1009,753,1024,736,1024 L,672,1024 C,655,1024,640,1009,640,992 L,640,768 L,416,768 C,399,768,384,753,384,736 L,384,672 C,384,655,399,640,416,640 L,640,640 L,640,416 C,640,399,655,384,672,384 L,736,384 C,753,384,768,399,768,416 L,768,640 L,992,640 C,1009,640,1024,655,1024,672 M,1152,704 C,1152,457,951,256,704,256 C,457,256,256,457,256,704 C,256,951,457,1152,704,1152 C,951,1152,1152,951,1152,704 M,1664,-128 C,1664,-94,1650,-61,1627,-38 L,1284,305 C,1365,422,1408,562,1408,704 C,1408,1093,1093,1408,704,1408 C,315,1408,0,1093,0,704 C,0,315,315,0,704,0 C,846,0,986,43,1103,124 L,1446,-218 C,1469,-242,1502,-256,1536,-256 C,1607,-256,1664,-199,1664,-128 Z"
		},
		"zoom_out": {
			"advanceWidth": 1664,
			"commands": "M,1024,736 C,1024,753,1009,768,992,768 L,416,768 C,399,768,384,753,384,736 L,384,672 C,384,655,399,640,416,640 L,992,640 C,1009,640,1024,655,1024,672 M,1152,704 C,1152,457,951,256,704,256 C,457,256,256,457,256,704 C,256,951,457,1152,704,1152 C,951,1152,1152,951,1152,704 M,1664,-128 C,1664,-94,1650,-61,1627,-38 L,1284,305 C,1365,422,1408,562,1408,704 C,1408,1093,1093,1408,704,1408 C,315,1408,0,1093,0,704 C,0,315,315,0,704,0 C,846,0,986,43,1103,124 L,1446,-218 C,1469,-242,1502,-256,1536,-256 C,1607,-256,1664,-199,1664,-128 Z"
		},
		"cog": {
			"advanceWidth": 1536,
			"commands": "M,1024,640 C,1024,499,909,384,768,384 C,627,384,512,499,512,640 C,512,781,627,896,768,896 C,909,896,1024,781,1024,640 M,1536,749 C,1536,766,1524,782,1507,785 L,1324,813 C,1314,846,1300,879,1283,911 C,1317,958,1354,1002,1388,1048 C,1393,1055,1396,1062,1396,1071 C,1396,1079,1394,1087,1389,1093 C,1347,1152,1277,1214,1224,1263 C,1217,1269,1208,1273,1199,1273 C,1190,1273,1181,1270,1175,1264 L,1033,1157 C,1004,1172,974,1184,943,1194 L,915,1378 C,913,1395,897,1408,879,1408 L,657,1408 C,639,1408,625,1396,621,1380 C,605,1320,599,1255,592,1194 C,561,1184,530,1171,501,1156 L,363,1263 C,355,1269,346,1273,337,1273 C,303,1273,168,1127,144,1094 C,139,1087,135,1080,135,1071 C,135,1062,139,1054,145,1047 C,182,1002,218,957,252,909 C,236,879,223,849,213,817 L,27,789 C,12,786,0,768,0,753 L,0,531 C,0,514,12,498,29,495 L,212,468 C,222,434,236,401,253,369 C,219,322,182,278,148,232 C,143,225,140,218,140,209 C,140,201,142,193,147,186 C,189,128,259,66,312,18 C,319,11,328,7,337,7 C,346,7,355,10,362,16 L,503,123 C,532,108,562,96,593,86 L,621,-98 C,623,-115,639,-128,657,-128 L,879,-128 C,897,-128,911,-116,915,-100 C,931,-40,937,25,944,86 C,975,96,1006,109,1035,124 L,1173,16 C,1181,11,1190,7,1199,7 C,1233,7,1368,154,1392,186 C,1398,193,1401,200,1401,209 C,1401,218,1397,227,1391,234 C,1354,279,1318,323,1284,372 C,1300,401,1312,431,1323,463 L,1508,491 C,1524,494,1536,512,1536,527 Z"
		},
		"trash": {
			"advanceWidth": 1408,
			"commands": "M,512,800 C,512,818,498,832,480,832 L,416,832 C,398,832,384,818,384,800 L,384,224 C,384,206,398,192,416,192 L,480,192 C,498,192,512,206,512,224 M,768,800 C,768,818,754,832,736,832 L,672,832 C,654,832,640,818,640,800 L,640,224 C,640,206,654,192,672,192 L,736,192 C,754,192,768,206,768,224 M,1024,800 C,1024,818,1010,832,992,832 L,928,832 C,910,832,896,818,896,800 L,896,224 C,896,206,910,192,928,192 L,992,192 C,1010,192,1024,206,1024,224 M,1152,76 C,1152,28,1125,0,1120,0 L,288,0 C,283,0,256,28,256,76 L,256,1024 L,1152,1024 L,1152,76 M,480,1152 L,529,1269 C,532,1273,540,1279,546,1280 L,863,1280 C,868,1279,877,1273,880,1269 L,928,1152 M,1408,1120 C,1408,1138,1394,1152,1376,1152 L,1067,1152 L,997,1319 C,977,1368,917,1408,864,1408 L,544,1408 C,491,1408,431,1368,411,1319 L,341,1152 L,32,1152 C,14,1152,0,1138,0,1120 L,0,1056 C,0,1038,14,1024,32,1024 L,128,1024 L,128,72 C,128,-38,200,-128,288,-128 L,1120,-128 C,1208,-128,1280,-34,1280,76 L,1280,1024 L,1376,1024 C,1394,1024,1408,1038,1408,1056 Z"
		},
		"file_alt": {
			"advanceWidth": 1536,
			"commands": "M,1468,1156 L,1156,1468 C,1119,1505,1045,1536,992,1536 L,96,1536 C,43,1536,0,1493,0,1440 L,0,-160 C,0,-213,43,-256,96,-256 L,1440,-256 C,1493,-256,1536,-213,1536,-160 L,1536,992 C,1536,1045,1505,1119,1468,1156 M,1024,1400 C,1041,1394,1058,1385,1065,1378 L,1378,1065 C,1385,1058,1394,1041,1400,1024 L,1024,1024 M,1408,-128 L,128,-128 L,128,1408 L,896,1408 L,896,992 C,896,939,939,896,992,896 L,1408,896 Z"
		},
		"download_alt": {
			"advanceWidth": 1664,
			"commands": "M,1280,192 C,1280,157,1251,128,1216,128 C,1181,128,1152,157,1152,192 C,1152,227,1181,256,1216,256 C,1251,256,1280,227,1280,192 M,1536,192 C,1536,157,1507,128,1472,128 C,1437,128,1408,157,1408,192 C,1408,227,1437,256,1472,256 C,1507,256,1536,227,1536,192 M,1664,416 C,1664,469,1621,512,1568,512 L,1104,512 L,968,376 C,931,340,883,320,832,320 C,781,320,733,340,696,376 L,561,512 L,96,512 C,43,512,0,469,0,416 L,0,96 C,0,43,43,0,96,0 L,1568,0 C,1621,0,1664,43,1664,96 M,1339,985 C,1329,1008,1306,1024,1280,1024 L,1024,1024 L,1024,1472 C,1024,1507,995,1536,960,1536 L,704,1536 C,669,1536,640,1507,640,1472 L,640,1024 L,384,1024 C,358,1024,335,1008,325,985 C,315,961,320,933,339,915 L,787,467 C,799,454,816,448,832,448 C,848,448,865,454,877,467 L,1325,915 C,1344,933,1349,961,1339,985 Z"
		},
		"repeat": {
			"advanceWidth": 1536,
			"commands": "M,1536,1280 C,1536,1306,1520,1329,1497,1339 C,1473,1349,1445,1344,1427,1325 L,1297,1196 C,1156,1329,965,1408,768,1408 C,345,1408,0,1063,0,640 C,0,217,345,-128,768,-128 C,997,-128,1213,-27,1359,149 C,1369,162,1369,181,1357,192 L,1220,330 C,1213,336,1204,339,1195,339 C,1186,338,1177,334,1172,327 C,1074,200,927,128,768,128 C,486,128,256,358,256,640 C,256,922,486,1152,768,1152 C,899,1152,1023,1102,1117,1015 L,979,877 C,960,859,955,831,965,808 C,975,784,998,768,1024,768 L,1472,768 C,1507,768,1536,797,1536,832 Z"
		},
		"pencil": {
			"advanceWidth": 1536,
			"commands": "M,363,0 L,256,0 L,256,128 L,128,128 L,128,235 L,219,326 L,454,91 M,886,928 C,886,922,884,916,879,911 L,337,369 C,332,364,326,362,320,362 C,307,362,298,371,298,384 C,298,390,300,396,305,401 L,847,943 C,852,948,858,950,864,950 C,877,950,886,941,886,928 M,832,1120 L,0,288 L,0,-128 L,416,-128 L,1248,704 M,1515,1024 C,1515,1058,1501,1091,1478,1115 L,1243,1349 C,1219,1373,1186,1387,1152,1387 C,1118,1387,1085,1373,1062,1349 L,896,1184 L,1312,768 L,1478,934 C,1501,957,1515,990,1515,1024 Z"
		},
		"edit": {
			"advanceWidth": 1792,
			"commands": "M,888,352 L,832,352 L,832,448 L,736,448 L,736,504 L,852,620 L,1004,468 M,1328,1072 C,1337,1063,1336,1048,1327,1039 L,977,689 C,968,680,953,679,944,688 C,935,697,936,712,945,721 L,1295,1071 C,1304,1080,1319,1081,1328,1072 M,1408,478 C,1408,491,1400,502,1388,507 C,1376,512,1363,510,1353,500 L,1289,436 C,1283,430,1280,422,1280,414 L,1280,288 C,1280,200,1208,128,1120,128 L,288,128 C,200,128,128,200,128,288 L,128,1120 C,128,1208,200,1280,288,1280 L,1120,1280 C,1135,1280,1150,1278,1165,1274 C,1176,1270,1188,1273,1197,1282 L,1246,1331 C,1254,1339,1257,1349,1255,1360 C,1253,1370,1246,1379,1237,1383 C,1200,1400,1160,1408,1120,1408 L,288,1408 C,129,1408,0,1279,0,1120 L,0,288 C,0,129,129,0,288,0 L,1120,0 C,1279,0,1408,129,1408,288 M,1312,1216 L,640,544 L,640,256 L,928,256 L,1600,928 M,1756,1084 C,1793,1121,1793,1183,1756,1220 L,1604,1372 C,1567,1409,1505,1409,1468,1372 L,1376,1280 L,1664,992 L,1756,1084 Z"
		},
		"play": {
			"advanceWidth": 1408,
			"commands": "M,1384,609 C,1415,626,1415,654,1384,671 L,56,1409 C,25,1426,0,1411,0,1376 L,0,-96 C,0,-131,25,-146,56,-129 Z"
		},
		"pause": {
			"advanceWidth": 1536,
			"commands": "M,1536,1344 C,1536,1379,1507,1408,1472,1408 L,960,1408 C,925,1408,896,1379,896,1344 L,896,-64 C,896,-99,925,-128,960,-128 L,1472,-128 C,1507,-128,1536,-99,1536,-64 M,640,1344 C,640,1379,611,1408,576,1408 L,64,1408 C,29,1408,0,1379,0,1344 L,0,-64 C,0,-99,29,-128,64,-128 L,576,-128 C,611,-128,640,-99,640,-64 Z"
		},
		"stop": {
			"advanceWidth": 1536,
			"commands": "M,1536,1344 C,1536,1379,1507,1408,1472,1408 L,64,1408 C,29,1408,0,1379,0,1344 L,0,-64 C,0,-99,29,-128,64,-128 L,1472,-128 C,1507,-128,1536,-99,1536,-64 Z"
		},
		"resize_full": {
			"advanceWidth": 1536,
			"commands": "M,755,480 C,755,488,751,497,745,503 L,631,617 C,625,623,616,627,608,627 C,600,627,591,623,585,617 L,253,285 L,109,429 C,97,441,81,448,64,448 C,29,448,0,419,0,384 L,0,-64 C,0,-99,29,-128,64,-128 L,512,-128 C,547,-128,576,-99,576,-64 C,576,-47,569,-31,557,-19 L,413,125 L,745,457 C,751,463,755,472,755,480 M,1536,1344 C,1536,1379,1507,1408,1472,1408 L,1024,1408 C,989,1408,960,1379,960,1344 C,960,1327,967,1311,979,1299 L,1123,1155 L,791,823 C,785,817,781,808,781,800 C,781,792,785,783,791,777 L,905,663 C,911,657,920,653,928,653 C,936,653,945,657,951,663 L,1283,995 L,1427,851 C,1439,839,1455,832,1472,832 C,1507,832,1536,861,1536,896 Z"
		},
		"resize_small": {
			"advanceWidth": 1536,
			"commands": "M,768,576 C,768,611,739,640,704,640 L,256,640 C,221,640,192,611,192,576 C,192,559,199,543,211,531 L,355,387 L,23,55 C,17,49,13,40,13,32 C,13,24,17,15,23,9 L,137,-105 C,143,-111,152,-115,160,-115 C,168,-115,177,-111,183,-105 L,515,227 L,659,83 C,671,71,687,64,704,64 C,739,64,768,93,768,128 M,1523,1248 C,1523,1256,1519,1265,1513,1271 L,1399,1385 C,1393,1391,1384,1395,1376,1395 C,1368,1395,1359,1391,1353,1385 L,1021,1053 L,877,1197 C,865,1209,849,1216,832,1216 C,797,1216,768,1187,768,1152 L,768,704 C,768,669,797,640,832,640 L,1280,640 C,1315,640,1344,669,1344,704 C,1344,721,1337,737,1325,749 L,1181,893 L,1513,1225 C,1519,1231,1523,1240,1523,1248 Z"
		},
		"eye_open": {
			"advanceWidth": 1792,
			"commands": "M,1664,576 C,1493,312,1217,128,896,128 C,575,128,299,312,128,576 C,223,723,353,849,509,929 C,469,861,448,783,448,704 C,448,457,649,256,896,256 C,1143,256,1344,457,1344,704 C,1344,783,1323,861,1283,929 C,1439,849,1569,723,1664,576 M,944,960 C,944,934,922,912,896,912 C,782,912,688,818,688,704 C,688,678,666,656,640,656 C,614,656,592,678,592,704 C,592,871,729,1008,896,1008 C,922,1008,944,986,944,960 M,1792,576 C,1792,601,1784,624,1772,645 C,1588,947,1251,1152,896,1152 C,541,1152,204,947,20,645 C,8,624,0,601,0,576 C,0,551,8,528,20,507 C,204,205,541,0,896,0 C,1251,0,1588,204,1772,507 C,1784,528,1792,551,1792,576 Z"
		},
		"eye_close": {
			"advanceWidth": 1792,
			"commands": "M,555,201 C,379,280,232,415,128,576 C,223,723,353,849,509,929 C,469,861,448,783,448,704 C,448,561,517,426,633,342 M,944,960 C,944,934,922,912,896,912 C,782,912,688,819,688,704 C,688,678,666,656,640,656 C,614,656,592,678,592,704 C,592,871,729,1008,896,1008 C,922,1008,944,986,944,960 M,1307,1151 C,1307,1162,1301,1172,1291,1178 C,1270,1190,1176,1248,1158,1248 C,1146,1248,1136,1242,1130,1232 L,1076,1135 C,1017,1146,956,1152,896,1152 C,527,1152,218,949,20,645 C,7,625,0,600,0,576 C,0,551,7,527,20,507 C,135,327,298,177,492,89 C,482,72,448,18,448,2 C,448,-10,454,-20,464,-26 C,485,-38,580,-96,598,-96 C,609,-96,620,-90,626,-80 L,675,9 C,886,386,1095,765,1306,1142 C,1307,1144,1307,1149,1307,1151 M,1344,704 C,1344,732,1341,760,1336,788 L,1056,286 C,1229,352,1344,518,1344,704 M,1792,576 C,1792,602,1785,623,1772,645 C,1694,774,1569,899,1445,982 L,1382,870 C,1495,792,1590,691,1664,576 C,1508,334,1261,157,970,132 L,896,0 C,1197,0,1467,137,1663,362 C,1702,407,1741,456,1772,507 C,1785,529,1792,550,1792,576 Z"
		},
		"folder_open": {
			"advanceWidth": 1920,
			"commands": "M,1879,584 C,1879,629,1828,640,1792,640 L,704,640 C,616,640,498,586,440,518 L,104,122 C,88,104,73,80,73,56 C,73,11,124,0,160,0 L,1248,0 C,1336,0,1454,54,1512,122 L,1848,518 C,1864,536,1879,560,1879,584 M,1536,928 C,1536,1051,1435,1152,1312,1152 L,768,1152 L,768,1184 C,768,1307,667,1408,544,1408 L,224,1408 C,101,1408,0,1307,0,1184 L,0,224 C,0,216,1,207,1,199 L,6,205 L,343,601 C,424,697,579,768,704,768 L,1536,768 Z"
		},
		"signin": {
			"advanceWidth": 1536,
			"commands": "M,1184,640 C,1184,657,1177,673,1165,685 L,621,1229 C,609,1241,593,1248,576,1248 C,541,1248,512,1219,512,1184 L,512,896 L,64,896 C,29,896,0,867,0,832 L,0,448 C,0,413,29,384,64,384 L,512,384 L,512,96 C,512,61,541,32,576,32 C,593,32,609,39,621,51 L,1165,595 C,1177,607,1184,623,1184,640 M,1536,992 C,1536,1151,1407,1280,1248,1280 L,928,1280 C,883,1280,896,1212,896,1184 C,896,1147,935,1152,960,1152 L,1248,1152 C,1336,1152,1408,1080,1408,992 L,1408,288 C,1408,200,1336,128,1248,128 L,928,128 C,883,128,896,60,896,32 C,896,15,911,0,928,0 L,1248,0 C,1407,0,1536,129,1536,288 Z"
		},
		"upload_alt": {
			"advanceWidth": 1664,
			"commands": "M,1280,64 C,1280,29,1251,0,1216,0 C,1181,0,1152,29,1152,64 C,1152,99,1181,128,1216,128 C,1251,128,1280,99,1280,64 M,1536,64 C,1536,29,1507,0,1472,0 C,1437,0,1408,29,1408,64 C,1408,99,1437,128,1472,128 C,1507,128,1536,99,1536,64 M,1664,288 C,1664,341,1621,384,1568,384 L,1141,384 C,1114,310,1043,256,960,256 L,704,256 C,621,256,550,310,523,384 L,96,384 C,43,384,0,341,0,288 L,0,-32 C,0,-85,43,-128,96,-128 L,1568,-128 C,1621,-128,1664,-85,1664,-32 M,1339,936 C,1349,959,1344,987,1325,1005 L,877,1453 C,865,1466,848,1472,832,1472 C,816,1472,799,1466,787,1453 L,339,1005 C,320,987,315,959,325,936 C,335,912,358,896,384,896 L,640,896 L,640,448 C,640,413,669,384,704,384 L,960,384 C,995,384,1024,413,1024,448 L,1024,896 L,1280,896 C,1306,896,1329,912,1339,936 Z"
		},
		"save": {
			"advanceWidth": 1536,
			"commands": "M,384,0 L,384,384 L,1152,384 L,1152,0 M,1280,0 L,1280,416 C,1280,469,1237,512,1184,512 L,352,512 C,299,512,256,469,256,416 L,256,0 L,128,0 L,128,1280 L,256,1280 L,256,864 C,256,811,299,768,352,768 L,928,768 C,981,768,1024,811,1024,864 L,1024,1280 C,1044,1280,1083,1264,1097,1250 L,1378,969 C,1391,956,1408,915,1408,896 L,1408,0 M,896,928 C,896,911,881,896,864,896 L,672,896 C,655,896,640,911,640,928 L,640,1248 C,640,1265,655,1280,672,1280 L,864,1280 C,881,1280,896,1265,896,1248 L,896,928 M,1536,896 C,1536,949,1506,1022,1468,1060 L,1188,1340 C,1150,1378,1077,1408,1024,1408 L,96,1408 C,43,1408,0,1365,0,1312 L,0,-32 C,0,-85,43,-128,96,-128 L,1440,-128 C,1493,-128,1536,-85,1536,-32 Z"
		},
		"undo": {
			"advanceWidth": 1536,
			"commands": "M,1536,640 C,1536,1063,1191,1408,768,1408 C,571,1408,380,1329,239,1196 L,109,1325 C,91,1344,63,1349,40,1339 C,16,1329,0,1306,0,1280 L,0,832 C,0,797,29,768,64,768 L,512,768 C,538,768,561,784,571,808 C,581,831,576,859,557,877 L,420,1015 C,513,1102,637,1152,768,1152 C,1050,1152,1280,922,1280,640 C,1280,358,1050,128,768,128 C,609,128,462,200,364,327 C,359,334,350,338,341,339 C,332,339,323,336,316,330 L,179,192 C,168,181,167,162,177,149 C,323,-27,539,-128,768,-128 C,1191,-128,1536,217,1536,640 Z"
		},
		"paste": {
			"advanceWidth": 1792,
			"commands": "M,768,-128 L,768,1024 L,1152,1024 L,1152,608 C,1152,555,1195,512,1248,512 L,1664,512 L,1664,-128 M,1024,1312 C,1024,1295,1009,1280,992,1280 L,288,1280 C,271,1280,256,1295,256,1312 L,256,1376 C,256,1393,271,1408,288,1408 L,992,1408 C,1009,1408,1024,1393,1024,1376 L,1024,1312 M,1280,640 L,1280,939 L,1579,640 M,1792,512 C,1792,565,1762,638,1724,676 L,1316,1084 C,1305,1095,1293,1104,1280,1112 L,1280,1440 C,1280,1493,1237,1536,1184,1536 L,96,1536 C,43,1536,0,1493,0,1440 L,0,96 C,0,43,43,0,96,0 L,640,0 L,640,-160 C,640,-213,683,-256,736,-256 L,1696,-256 C,1749,-256,1792,-213,1792,-160 Z"
		},
		"folder_open_alt": {
			"advanceWidth": 1920,
			"commands": "M,1781,605 C,1781,590,1772,577,1763,566 L,1469,203 C,1435,161,1365,128,1312,128 L,224,128 C,202,128,171,135,171,163 C,171,178,180,191,189,203 L,483,566 C,517,607,587,640,640,640 L,1728,640 C,1750,640,1781,633,1781,605 M,640,768 C,549,768,442,717,384,646 L,128,331 L,128,1184 C,128,1237,171,1280,224,1280 L,544,1280 C,597,1280,640,1237,640,1184 L,640,1120 C,640,1067,683,1024,736,1024 L,1312,1024 C,1365,1024,1408,981,1408,928 L,1408,768 M,1909,605 C,1909,629,1904,652,1894,673 C,1864,737,1796,768,1728,768 L,1536,768 L,1536,928 C,1536,1051,1435,1152,1312,1152 L,768,1152 L,768,1184 C,768,1307,667,1408,544,1408 L,224,1408 C,101,1408,0,1307,0,1184 L,0,224 C,0,101,101,0,224,0 L,1312,0 C,1402,0,1511,52,1568,122 L,1863,485 C,1890,519,1909,561,1909,605 Z"
		}
	}
};

const { style: style$2 } = utils;

function IconButton(size, icon, tooltip, dispatcher) {
	var iconStyle = {
		padding: '0.2em 0.4em',
		margin: '0em',
		background: 'none',
		outline: 'none',
		fontSize: '16px',
		border: 'none',
		borderRadius: '0.2em',
	};

	var button = document.createElement('button');
	style$2(button, iconStyle);

	var canvas = document.createElement('canvas');
	var ctx = canvas.getContext('2d');

	button.appendChild(canvas);

	this.ctx = ctx;
	this.dom = button;
	this.canvas = canvas;

	var me = this;
	this.size = size;
	var dpr = 1;

	this.resize = function() {
		dpr = window.devicePixelRatio;
		var height = size;

		var glyph = font.fonts[icon];

		canvas.height = height * dpr;
		canvas.style.height = height + 'px';

		var scale = height / font.unitsPerEm;
		var width = glyph.advanceWidth * scale + 0.5 | 0;

		width += 2;
		height += 2;

		canvas.width = width * dpr;
		canvas.style.width = width + 'px';

		ctx.fillStyle = Theme.c;
		me.draw();
	};

	if (dispatcher) dispatcher.on('resize', this.resize);

	this.setSize = function(s) {
		size = s;
		this.resize();
	};

	this.setIcon = function(icon) {
		me.icon = icon;

		if (!font.fonts[icon]) console.warn('Font icon not found!');
		this.resize();
	};

	this.onClick = function(e) {
		button.addEventListener('click', e);
	};

	var LONG_HOLD_DURATION = 500;
	var longHoldTimer;

	this.onLongHold = function(f) {
		// not most elagent but oh wells.
		function startHold(e) {
			e.preventDefault();
			e.stopPropagation();
			longHoldTimer = setTimeout(function() {
				if (longHoldTimer) {
					console.log('LONG HOLD-ED!');
					f();
				}
			}, LONG_HOLD_DURATION);
		}

		function clearLongHoldTimer() {
			clearTimeout(longHoldTimer);
		}

		button.addEventListener('mousedown', startHold);
		button.addEventListener('touchstart', startHold);
		button.addEventListener('mouseup', clearLongHoldTimer);
		button.addEventListener('mouseout', clearLongHoldTimer);
		button.addEventListener('touchend', clearLongHoldTimer);
	};

	this.setTip = function(tip) {
		tooltip = tip;
	};

	var borders = {
		border: '1px solid ' + Theme.b,
		// boxShadow: Theme.b + ' 1px 1px'
	};

	var no_borders = {
		border: '1px solid transparent',
		// boxShadow: 'none'
	};

	var normal = 'none'; // Theme.b;
	var down = Theme.b;

	button.style.background = normal;
	style$2(button, no_borders);

	button.addEventListener('mouseover', function() {
		// button.style.background = up;
		style$2(button, borders);

		ctx.fillStyle = Theme.d;
		// me.dropshadow = true;
		ctx.shadowColor = Theme.b;
		ctx.shadowBlur = 0.5 * dpr;
		ctx.shadowOffsetX = 1 * dpr;
		ctx.shadowOffsetY = 1 * dpr;
		me.draw();

		if (tooltip && dispatcher) dispatcher.fire('status', 'button: ' + tooltip);
	});

	button.addEventListener('mousedown', function() {
		button.style.background = down;
		// ctx.fillStyle = Theme.b;
		// me.draw();
	});

	button.addEventListener('mouseup', function() {
		// ctx.fillStyle = Theme.d;
		button.style.background = normal;
		style$2(button, borders);
		// me.draw();
	});

	button.addEventListener('mouseout', function() {
		// ctx.fillStyle = Theme.c;


		button.style.background = normal;
		style$2(button, no_borders);
		me.dropshadow = false;
		ctx.fillStyle = Theme.c;
		ctx.shadowColor = null;
		ctx.shadowBlur = 0;
		ctx.shadowOffsetX = 0;
		ctx.shadowOffsetY = 0;
		me.draw();
	});

	if (icon) this.setIcon(icon);
}

IconButton.prototype.CMD_MAP = {
	M: 'moveTo',
	L: 'lineTo',
	Q: 'quadraticCurveTo',
	C: 'bezierCurveTo',
	Z: 'closePath'
};

IconButton.prototype.draw = function() {
	if (!this.icon) return;

	var ctx = this.ctx;

	var glyph = font.fonts[this.icon];

	var height = this.size;
	var dpr = window.devicePixelRatio;
	var scale = height / font.unitsPerEm * dpr;
	var path_commands =  glyph.commands.split(' ');

	ctx.save();
	ctx.clearRect(0, 0, this.canvas.width * dpr, this.canvas.height * dpr);

	if (this.dropshadow) {
		ctx.save();
		ctx.fillStyle = Theme.b;
		ctx.translate(1.5 * dpr, 1.5 * dpr);
		ctx.scale(scale, -scale);
		ctx.translate(0 , -font.ascender);
		ctx.beginPath();

		for (let i = 0, il = path_commands.length; i < il; i++) {
			const cmds = path_commands[i].split(',');
			const params = cmds.slice(1);

			ctx[this.CMD_MAP[cmds[0]]].apply(ctx, params);
		}
		ctx.fill();
		ctx.restore();
	}

	ctx.scale(scale, -scale);
	ctx.translate(0, -font.ascender);
	ctx.beginPath();

	for (let i = 0, il = path_commands.length; i < il; i++) {
		const cmds = path_commands[i].split(',');
		const params = cmds.slice(1);

		ctx[this.CMD_MAP[cmds[0]]].apply(ctx, params);
	}
	ctx.fill();
	ctx.restore();

	/*
	var triangle = height / 3 * dpr;
	ctx.save();
	// ctx.translate(dpr * 2, 0);
	// ctx.fillRect(this.canvas.width - triangle, this.canvas.height - triangle, triangle, triangle);
	ctx.beginPath();
	ctx.moveTo(this.canvas.width - triangle, this.canvas.height - triangle / 2);
	ctx.lineTo(this.canvas.width, this.canvas.height - triangle / 2);
	ctx.lineTo(this.canvas.width - triangle / 2, this.canvas.height);
	ctx.fill();
	ctx.restore();
	*/
};

const { STORAGE_PREFIX: STORAGE_PREFIX$1, style: style$1 } = utils;

function LayerCabinet(data, dispatcher) {
	var layer_store = data.get('layers');

	var div = document.createElement('div');

	var top = document.createElement('div');
	top.style.cssText = 'margin: 0px; top: 0; left: 0; height: ' + LayoutConstants.MARKER_TRACK_HEIGHT + 'px';
	// top.style.textAlign = 'right';

	var layer_scroll = document.createElement('div');
	style$1(layer_scroll, {
		position: 'absolute',
		top: LayoutConstants.MARKER_TRACK_HEIGHT + 'px',
		// height: (LayoutConstants.height - LayoutConstants.MARKER_TRACK_HEIGHT) + 'px'
		left: 0,
		right: 0,
		bottom: 0,
		overflow: 'hidden'
	});

	layer_scroll.id = 'layer_scroll';

	div.appendChild(layer_scroll);

	var playing = false;


	var button_styles = {
		width: '22px',
		height: '22px',
		padding: '2px'
	};

	var op_button_styles = {
		width: '32px',
		padding: '3px 4px 3px 4px'
	};


	var play_button = new IconButton(16, 'play', 'play', dispatcher);
	style$1(play_button.dom, button_styles, { marginTop: '2px' } );
	play_button.onClick(function(e) {
		e.preventDefault();
		dispatcher.fire('controls.toggle_play');
	});

	var stop_button = new IconButton(16, 'stop', 'stop', dispatcher);
	style$1(stop_button.dom, button_styles, { marginTop: '2px' } );
	stop_button.onClick(function(e) {
		dispatcher.fire('controls.stop');
	});


	// var undo_button = new IconButton(16, 'undo', 'undo', dispatcher);
	// style$1(undo_button.dom, op_button_styles);
	// undo_button.onClick(function() {
	// 	dispatcher.fire('controls.undo');
	// });

	// var redo_button = new IconButton(16, 'repeat', 'redo', dispatcher);
	// style$1(redo_button.dom, op_button_styles);
	// redo_button.onClick(function() {
	// 	dispatcher.fire('controls.redo');
	// });

	var range = document.createElement('input');
	range.type = "range";
	range.value = 0;
	range.min = -1;
	range.max = +1;
	range.step = 0.125;

	style$1(range, {
		width: '90px',
		margin: '0px',
		marginLeft: '2px',
		marginRight: '2px'
	});

	var draggingRange = 0;

	range.addEventListener('mousedown', function() {
		draggingRange = 1;
	});

	range.addEventListener('mouseup', function() {
		draggingRange = 0;
		changeRange();
	});

	range.addEventListener('mousemove', function() {
		if (!draggingRange) return;
		changeRange();
	});

	div.appendChild(top);

	var time_options = {
		min: 0,
		step: 0.125
	};

	var currentTime = new UINumber(time_options);
	var totalTime = new UINumber(time_options);

	var currentTimeStore = data.get('ui:currentTime');
	var totalTimeStore = data.get('ui:totalTime');

	// UI2StoreBind(view, datastore) {
	// 	view.onChange.do(function(v) {
	// 		datastore.value = view;
	// 	})

	// 	datastore.onChange.do(function(v) {
	// 		view.setValue = v;
	// 	})
	// }

	currentTime.onChange.do(function(value, done) {
		dispatcher.fire('time.update', value);
		// repaint();
	});

	totalTime.onChange.do(function(value, done) {
		totalTimeStore.value = value;
		repaint();
	});

	// Play Controls
	top.appendChild(currentTime.dom);
	top.appendChild(document.createTextNode('/')); // 0:00:00 / 0:10:00
	top.appendChild(totalTime.dom);
	top.appendChild(play_button.dom);
	top.appendChild(stop_button.dom);
	top.appendChild(range);


	var operations_div = document.createElement('div');
	style$1(operations_div, {
		marginTop: '4px',
		// borderBottom: '1px solid ' + Theme.b
	});
	top.appendChild(operations_div);
	// top.appendChild(document.createElement('br'));


	// open _alt
	// var file_open = new IconButton(16, 'folder_open_alt', 'Open', dispatcher);
	// style$1(file_open.dom, op_button_styles);
	// operations_div.appendChild(file_open.dom);

	// function populateOpen() {
	// 	while (dropdown.length) {
	// 		dropdown.remove(0);
	// 	}

	// 	var option;
	// 	option = document.createElement('option');
	// 	option.text = 'New';
	// 	option.value = '*new*';
	// 	dropdown.add(option);

	// 	option = document.createElement('option');
	// 	option.text = 'Import JSON';
	// 	option.value = '*import*';
	// 	dropdown.add(option);

	// 	// Doesn't work
	// 	// option = document.createElement('option');
	// 	// option.text = 'Select File';
	// 	// option.value = '*select*';
	// 	// dropdown.add(option);

	// 	option = document.createElement('option');
	// 	option.text = '==Open==';
	// 	option.disabled = true;
	// 	option.selected = true;
	// 	dropdown.add(option);

	// 	var regex = new RegExp(STORAGE_PREFIX$1 + '(.*)');
	// 	for (var key in localStorage) {
	// 		// console.log(key);

	// 		var match = regex.exec(key);
	// 		if (match) {
	// 			option = document.createElement('option');
	// 			option.text = match[1];

	// 			dropdown.add(option);
	// 		}
	// 	}

	// }

	// listen on other tabs
	// window.addEventListener('storage', function(e) {
	// 	var regex = new RegExp(STORAGE_PREFIX$1 + '(.*)');
	// 	if (regex.exec(e.key)) {
	// 		populateOpen();
	// 	}
	// });

	// dispatcher.on('save:done', populateOpen);

	// var dropdown = document.createElement('select');

	// style$1(dropdown, {
	// 	position: 'absolute',
	// 	// right: 0,
	// 	// margin: 0,
	// 	opacity: 0,
	// 	width: '16px',
	// 	height: '16px',
	// 	// zIndex: 1,
	// });

	// dropdown.addEventListener('change', function(e) {
	// 	// console.log('changed', dropdown.length, dropdown.value);

	// 	switch (dropdown.value) {
	// 	case '*new*':
	// 		dispatcher.fire('new');
	// 		break;
	// 	case '*import*':
	// 		dispatcher.fire('import');
	// 		break;
	// 	case '*select*':
	// 		dispatcher.fire('openfile');
	// 		break;
	// 	default:
	// 		dispatcher.fire('open', dropdown.value);
	// 		break;
	// 	}
	// });

	// file_open.dom.insertBefore(dropdown, file_open.dom.firstChild);

	// populateOpen();

	// // json import
	// var import_json = new IconButton(16, 'signin', 'Import JSON', dispatcher);
	// operations_div.appendChild(import_json.dom);
	// import_json.onClick(function() {
	// 	dispatcher.fire('import');
	// });

	// // new
	// var file_alt = new IconButton(16, 'file_alt', 'New', dispatcher);
	// operations_div.appendChild(file_alt.dom);

	// save
	// var save = new IconButton(16, 'save', 'Save', dispatcher);
	// style$1(save.dom, op_button_styles);
	// operations_div.appendChild(save.dom);
	// save.onClick(function() {
	// 	dispatcher.fire('save');
	// });

	// save as
	// var save_as = new IconButton(16, 'paste', 'Save as', dispatcher);
	// style$1(save_as.dom, op_button_styles);
	// operations_div.appendChild(save_as.dom);
	// save_as.onClick(function() {
	// 	dispatcher.fire('save_as');
	// });

	// download json (export)
	// var download_alt = new IconButton(16, 'download_alt', 'Download / Export JSON to file', dispatcher);
	// style$1(download_alt.dom, op_button_styles);
	// operations_div.appendChild(download_alt.dom);
	// download_alt.onClick(function() {
	// 	dispatcher.fire('export');
	// });

	// var upload_alt = new IconButton(16, 'upload_alt', 'Load from file', dispatcher);
	// style$1(upload_alt.dom, op_button_styles);
	// operations_div.appendChild(upload_alt.dom);
	// upload_alt.onClick(function() {
	// 	dispatcher.fire('openfile');
	// });

	var span = document.createElement('span');
	span.style.width = '20px';
	span.style.display = 'inline-block';
	operations_div.appendChild(span);

	// operations_div.appendChild(undo_button.dom);
	// operations_div.appendChild(redo_button.dom);
	operations_div.appendChild(document.createElement('br'));

	function changeRange() {

		dispatcher.fire('update.scale', 6 * Math.pow(100, -range.value) );
	}

	var layer_uis = [];
	var unused_layers = [];

	this.layers = layer_uis;

	this.setControlStatus = function(v) {
		playing = v;
		if (playing) {
			play_button.setIcon('pause');
			play_button.setTip('Pause');
		}
		else {
			play_button.setIcon('play');
			play_button.setTip('Play');
		}
	};

	this.setState = function(state) {

		layer_store = state;
		var layers = layer_store.value;
		// layers = state;
		// console.log(layer_uis.length, layers);
		var i, layer;
		for (i = 0; i < layers.length; i++) {
			layer = layers[i];

			if (!layer_uis[i]) {
				var layer_ui;
				if (unused_layers.length) {
					layer_ui = unused_layers.pop();
					layer_ui.dom.style.display = 'block';
				} else {
					// new
					layer_ui = new LayerView(layer, dispatcher);
					layer_scroll.appendChild(layer_ui.dom);
				}
				layer_uis.push(layer_ui);
			}

			// layer_uis[i].setState(layer);
		}

		console.log('Total layers (view, hidden, total)', layer_uis.length, unused_layers.length,
			layer_uis.length + unused_layers.length);

	};

	function repaint(s) {

		s = currentTimeStore.value;
		currentTime.setValue(s);
		totalTime.setValue(totalTimeStore.value);
		currentTime.paint();
		totalTime.paint();

		var i;

		s = s || 0;

		var layers = layer_store.value;
		for (i = layer_uis.length; i-- > 0;) {
			// quick hack
			if (i >= layers.length) {
				layer_uis[i].dom.style.display = 'none';
				unused_layers.push(layer_uis.pop());
				continue;
			}

			layer_uis[i].setState(layers[i], layer_store.get(i));
			// layer_uis[i].setState('layers'+':'+i);
			layer_uis[i].repaint(s);
		}

		layer_uis.length;

	}

	this.repaint = repaint;
	this.setState(layer_store);

	this.scrollTo = function(x) {
		layer_scroll.scrollTop = x * (layer_scroll.scrollHeight - layer_scroll.clientHeight);
	};

	this.dom = div;

	repaint();
}

/* This is the top bar where it shows a horizontal scrolls as well as a custom view port */

function Rect() {

}

Rect.prototype.set = function(x, y, w, h, color, outline) {
	this.x = x;
	this.y = y;
	this.w = w;
	this.h = h;
	this.color = color;
	this.outline = outline;
};

Rect.prototype.paint = function(ctx) {
	ctx.fillStyle = Theme.b;  // // 'yellow';
	ctx.strokeStyle = Theme.c;

	this.shape(ctx);

	ctx.stroke();
	ctx.fill();
};

Rect.prototype.shape = function(ctx) {
	ctx.beginPath();
	ctx.rect(this.x, this.y, this.w, this.h);
};

Rect.prototype.contains = function(x, y) {
	return x >= this.x && y >= this.y && x <= this.x + this.w && y <= this.y + this.h;
};



function ScrollCanvas(dispatcher, data) {
	var width, height;

	this.setSize = function(w, h) {
		width = w;
		height = h;
	};
	var MARGINS = 15;

	var scroller = {
		left: 0,
		grip_length: 0,
		k: 1
	};

	var scrollRect = new Rect();

	this.paint = function(ctx) {
		var totalTime = data.get('ui:totalTime').value;
		var scrollTime = data.get('ui:scrollTime').value;
		var currentTime = data.get('ui:currentTime').value;

		var pixels_per_second = data.get('ui:timeScale').value;

		ctx.save();
		var dpr = window.devicePixelRatio;
		ctx.scale(dpr, dpr);

		var w = width - 2 * MARGINS;
		var h = 16; // TOP_SCROLL_TRACK;

		ctx.clearRect(0, 0, width, height);
		ctx.translate(MARGINS, 5);

		// outline scroller
		ctx.beginPath();
		ctx.strokeStyle = Theme.b;
		ctx.rect(0, 0, w, h);
		ctx.stroke();

		var totalTimePixels = totalTime * pixels_per_second;
		var k = w / totalTimePixels;
		scroller.k = k;

		var grip_length = w * k;

		scroller.grip_length = grip_length;

		scroller.left = scrollTime / totalTime * w;

		scrollRect.set(scroller.left, 0, scroller.grip_length, h);
		scrollRect.paint(ctx);

		var r = currentTime / totalTime * w;

		ctx.fillStyle =  Theme.c;
		ctx.lineWidth = 2;

		ctx.beginPath();

		// circle
		// ctx.arc(r, h2 / 2, h2 / 1.5, 0, Math.PI * 2);

		// line
		ctx.rect(r, 0, 2, h + 5);
		ctx.fill();

		ctx.fillText(currentTime && currentTime.toFixed(2), r, h + 14);
		// ctx.fillText(currentTime && currentTime.toFixed(3), 10, 10);
		ctx.fillText(totalTime, 300, 14);

		ctx.restore();
	};

	/** Handles dragging for scroll bar **/

	var draggingx = null;

	this.onDown = function(e) {
		// console.log('ondown', e);

		if (scrollRect.contains(e.offsetx - MARGINS, e.offsety -5)) {
			draggingx = scroller.left;
			return;
		}

		var totalTime = data.get('ui:totalTime').value;
		data.get('ui:timeScale').value;
		var w = width - 2 * MARGINS;

		var t = (e.offsetx - MARGINS) / w * totalTime;
		// t = Math.max(0, t);

		// data.get('ui:currentTime').value = t;
		dispatcher.fire('time.update', t);

		if (e.preventDefault) e.preventDefault();

	};

	this.onMove = function move(e) {
		if (draggingx != null) {
			var totalTime = data.get('ui:totalTime').value;
			var w = width - 2 * MARGINS;
			var scrollTime = (draggingx + e.dx) / w * totalTime;

			// console.log(scrollTime, draggingx, e.dx, scroller.grip_length, w);

			if (draggingx  + e.dx + scroller.grip_length > w) return;

			dispatcher.fire('update.scrollTime', scrollTime);

		} else {
			this.onDown(e);
		}

	};

	this.onUp = function(e) {
		draggingx = null;
	};

	/*** End handling for scrollbar ***/
}

function Canvas(w, h) {

	var canvas, ctx, width, height, dpr;

	var canvasItems = [];
	var child;

	function create() {
		canvas = document.createElement('canvas');
		ctx = canvas.getContext('2d');
	}

	function setSize(w, h) {
		width = w;
		height = h;
		dpr = window.devicePixelRatio;
		canvas.width = width * dpr;
		canvas.height = height * dpr;
		canvas.style.width = width + 'px';
		canvas.style.height = height + 'px';

		if (child) child.setSize(w, h);
	}

	function paint(ctx) {
		if (child) {
			if (!child.paint) console.warn('implement repaint()');
			child.paint(ctx);
		}

		var item;
		for (var i = 0; i < canvasItems.length; i++) {
			item = canvasItems[i];
			item.paint();
		}
	}

	function repaint() {
		paint(ctx);
	}

	function uses(c) {
		child = c;
		child.add = this.add;
		child.remove = this.remove;
	}

	create();
	setSize(w, h);
	this.setSize = setSize;
	this.repaint = repaint;
	this.uses = uses;

	this.dom = canvas;

	handleDrag(canvas,
		function down(e) {
			if (child.onDown) { child.onDown(e); }
		},
		function move(e) {
			if (child.onMove) { child.onMove(e); }
		},
		function up(e) {
			if (child.onUp) { child.onUp(e); }
		}
		// function hit(e) {
		// 	if (child.onHit) { child.onHit(e) };
		// }
	);
}

/*
 * Usage: canvas = new Canvas(width, height);
 * canvas.resize();
 */

// children
// 1: override repaint
// 2: add objects
// Canvas.uses(CanvasChild);
// CanvasItem
// width, height, x, y
// allow Drag
// allow Click
// mouseOver
//

const proxy_ctx  = utils.proxy_ctx;

var LINE_HEIGHT = LayoutConstants.LINE_HEIGHT,
	DIAMOND_SIZE = LayoutConstants.DIAMOND_SIZE,
	TIME_SCROLLER_HEIGHT = 35,
	MARKER_TRACK_HEIGHT = 25,
	time_scale = LayoutConstants.time_scale;


var frame_start = 0; // this is the current scroll position.


/*
 * This class contains the view for the right main section of timeliner
 */


// TODO
// dirty rendering
// drag block
// DON'T use time.update for everything

var tickMark1;
var tickMark2;
var tickMark3;

function time_scaled() {
	/*
	 * Subdivison LOD
	 * time_scale refers to number of pixels per unit
	 * Eg. 1 inch - 60s, 1 inch - 60fps, 1 inch - 6 mins
	 */
	var div = 60;

	tickMark1 = time_scale / div;
	tickMark2 = 2 * tickMark1;
	tickMark3 = 10 * tickMark1;

}

time_scaled();


/**************************/
// Timeline Panel
/**************************/

function TimelinePanel(data, dispatcher) {

	var dpr = window.devicePixelRatio;
	var track_canvas = document.createElement('canvas');

	var scrollTop = 0, scrollLeft = 0, SCROLL_HEIGHT;
	var layers = data.get('layers').value;

	this.scrollTo = function(s, y) {
		scrollTop = s * Math.max(layers.length * LINE_HEIGHT - SCROLL_HEIGHT, 0);
		repaint();
	};

	this.resize = function() {
		var h = (LayoutConstants.height - TIME_SCROLLER_HEIGHT);
		dpr = window.devicePixelRatio;
		track_canvas.width = LayoutConstants.width * dpr;
		track_canvas.height = h * dpr;
		track_canvas.style.width = LayoutConstants.width + 'px';
		track_canvas.style.height = h + 'px';
		SCROLL_HEIGHT = LayoutConstants.height - TIME_SCROLLER_HEIGHT;
		scroll_canvas.setSize(LayoutConstants.width, TIME_SCROLLER_HEIGHT);
	};

	var div = document.createElement('div');

	var scroll_canvas = new Canvas(LayoutConstants.width, TIME_SCROLLER_HEIGHT);
	// data.addListener('ui', repaint );

	utils.style(track_canvas, {
		position: 'absolute',
		top: TIME_SCROLLER_HEIGHT + 'px',
		left: '0px'
	});

	utils.style(scroll_canvas.dom, {
		position: 'absolute',
		top: '0px',
		left: '10px'
	});

	scroll_canvas.uses(new ScrollCanvas(dispatcher, data));

	div.appendChild(track_canvas);
	div.appendChild(scroll_canvas.dom);
	scroll_canvas.dom.id = 'scroll-canvas';
	track_canvas.id = 'track-canvas';

	// this.dom = canvas;
	this.dom = div;
	this.dom.id = 'timeline-panel';
	this.resize();

	var ctx = track_canvas.getContext('2d');
	var ctx_wrap = proxy_ctx(ctx);

	var currentTime; // measured in seconds
	// technically it could be in frames or  have it in string format (0:00:00:1-60)

	var LEFT_GUTTER = 20;
	var i, x, y, il;

	var needsRepaint = false;
	var renderItems = [];

	function EasingRect(x1, y1, x2, y2, frame, frame2, values, layer, j) {

		this.path = function() {
			ctx_wrap.beginPath()
				.rect(x1, y1, x2-x1, y2-y1)
				.closePath();
		};

		this.paint = function() {
			this.path();
			ctx.fillStyle = frame._color;
			ctx.fill();
		};

		this.mouseover = function() {
			track_canvas.style.cursor = 'pointer'; // pointer move ew-resize
		};

		this.mouseout = function() {
			track_canvas.style.cursor = 'default';
		};

		this.mousedrag = function(e) {
			var t1 = x_to_time(x1 + e.dx);
			t1 = Math.max(0, t1);
			// TODO limit moving to neighbours
			frame.time = t1;

			var t2 = x_to_time(x2 + e.dx);
			t2 = Math.max(0, t2);
			frame2.time = t2;

			// dispatcher.fire('time.update', t1);
		};
	}

	function Diamond(frame, y) {
		var x, y2;

		x = time_to_x(frame.time);
		y2 = y + LINE_HEIGHT * 0.5  - DIAMOND_SIZE / 2;

		var self = this;

		var isOver = false;

		this.path = function(ctx_wrap) {
			ctx_wrap
				.beginPath()
				.moveTo(x, y2)
				.lineTo(x + DIAMOND_SIZE / 2, y2 + DIAMOND_SIZE / 2)
				.lineTo(x, y2 + DIAMOND_SIZE)
				.lineTo(x - DIAMOND_SIZE / 2, y2 + DIAMOND_SIZE / 2)
				.closePath();
		};

		this.paint = function(ctx_wrap) {
			self.path(ctx_wrap);
			if (!isOver)
				ctx_wrap.fillStyle(Theme.c);
			else
				ctx_wrap.fillStyle('yellow'); // Theme.d

			ctx_wrap.fill()
				.stroke();
		};

		this.mouseover = function() {
			isOver = true;
			track_canvas.style.cursor = 'move'; // pointer move ew-resize
			self.paint(ctx_wrap);
		};

		this.mouseout = function() {
			isOver = false;
			track_canvas.style.cursor = 'default';
			self.paint(ctx_wrap);
		};

		this.mousedrag = function(e) {
			var t = x_to_time(x + e.dx);
			t = Math.max(0, t);
			// TODO limit moving to neighbours
			frame.time = t;
			dispatcher.fire('time.update', t);
			// console.log('frame', frame);
			// console.log(s, format_friendly_seconds(s), this);
		};

	}

	function repaint() {
		needsRepaint = true;
	}


	function drawLayerContents() {
		renderItems = [];
		// horizontal Layer lines
		for (i = 0, il = layers.length; i <= il; i++) {
			ctx.strokeStyle = Theme.b;
			ctx.beginPath();
			y = i * LINE_HEIGHT;
			y = ~~y - 0.5;

			ctx_wrap
				.moveTo(0, y)
				.lineTo(LayoutConstants.width, y)
				.stroke();
		}


		var frame, frame2, j;

		// Draw Easing Rects
		for (i = 0; i < il; i++) {
			// check for keyframes
			var layer = layers[i];
			var values = layer.values;

			y = i * LINE_HEIGHT;

			for (j = 0; j < values.length - 1; j++) {
				frame = values[j];
				frame2 = values[j + 1];

				// Draw Tween Rect
				var x = time_to_x(frame.time);
				var x2 = time_to_x(frame2.time);

				if (!frame.tween || frame.tween == 'none') continue;

				var y1 = y + 2;
				var y2 = y + LINE_HEIGHT - 2;

				renderItems.push(new EasingRect(x, y1, x2, y2, frame, frame2));

				// // draw easing graph
				// var color = parseInt(frame._color.substring(1,7), 16);
				// color = 0xffffff ^ color;
				// color = color.toString(16);           // convert to hex
				// color = '#' + ('000000' + color).slice(-6);

				// ctx.strokeStyle = color;
				// var x3;
				// ctx.beginPath();
				// ctx.moveTo(x, y2);
				// var dy = y1 - y2;
				// var dx = x2 - x;

				// for (x3=x; x3 < x2; x3++) {
				// 	ctx.lineTo(x3, y2 + Tweens[frame.tween]((x3 - x)/dx) * dy);
				// }
				// ctx.stroke();
			}

			for (j = 0; j < values.length; j++) {
				// Dimonds
				frame = values[j];
				renderItems.push(new Diamond(frame, y));
			}
		}

		// render items
		var item;
		for (i = 0, il = renderItems.length; i < il; i++) {
			item = renderItems[i];
			item.paint(ctx_wrap);
		}
	}

	function setTimeScale() {

		var v = data.get('ui:timeScale').value;
		if (time_scale !== v) {
			time_scale = v;
			time_scaled();
		}
	}

	var over = null;
	var mousedownItem = null;

	function check() {
		var item;
		var last_over = over;
		// over = [];
		over = null;
		for (i = renderItems.length; i-- > 0;) {
			item = renderItems[i];
			item.path(ctx_wrap);

			if (ctx.isPointInPath(pointer.x * dpr, pointer.y * dpr)) {
				// over.push(item);
				over = item;
				break;
			}
		}

		// clear old mousein
		if (last_over && last_over != over) {
			item = last_over;
			if (item.mouseout) item.mouseout();
		}

		if (over) {
			item = over;
			if (item.mouseover) item.mouseover();

			if (mousedown2) {
				mousedownItem = item;
			}
		}



		// console.log(pointer)
	}

	function pointerEvents() {
		if (!pointer) return;

		ctx_wrap
			.save()
			.scale(dpr, dpr)
			.translate(0, MARKER_TRACK_HEIGHT)
			.beginPath()
			.rect(0, 0, LayoutConstants.width, SCROLL_HEIGHT)
			.translate(-scrollLeft, -scrollTop)
			.clip()
			.run(check)
			.restore();
	}

	function _paint() {
		if (!needsRepaint) {
			pointerEvents();
			return;
		}

		scroll_canvas.repaint();

		setTimeScale();

		currentTime = data.get('ui:currentTime').value;
		frame_start =  data.get('ui:scrollTime').value;

		/**************************/
		// background

		ctx.fillStyle = Theme.a;
		ctx.clearRect(0, 0, track_canvas.width, track_canvas.height);
		ctx.save();
		ctx.scale(dpr, dpr);

		//

		ctx.lineWidth = 1; // .5, 1, 2

		var width = LayoutConstants.width;
		var height = LayoutConstants.height;

		var units = time_scale / tickMark1;
		var offsetUnits = (frame_start * time_scale) % units;

		var count = (width - LEFT_GUTTER + offsetUnits) / units;

		// console.log('time_scale', time_scale, 'tickMark1', tickMark1, 'units', units, 'offsetUnits', offsetUnits, frame_start);

		// time_scale = pixels to 1 second (40)
		// tickMark1 = marks per second (marks / s)
		// units = pixels to every mark (40)

		// labels only
		for (i = 0; i < count; i++) {
			x = i * units + LEFT_GUTTER - offsetUnits;

			// vertical lines
			ctx.strokeStyle = Theme.b;
			ctx.beginPath();
			ctx.moveTo(x, 0);
			ctx.lineTo(x, height);
			ctx.stroke();

			ctx.fillStyle = Theme.d;
			ctx.textAlign = 'center';

			var t = (i * units - offsetUnits) / time_scale + frame_start;
			t = utils.format_friendly_seconds(t);
			ctx.fillText(t, x, 38);
		}

		units = time_scale / tickMark2;
		count = (width - LEFT_GUTTER + offsetUnits) / units;

		// marker lines - main
		for (i = 0; i < count; i++) {
			ctx.strokeStyle = Theme.c;
			ctx.beginPath();
			x = i * units + LEFT_GUTTER - offsetUnits;
			ctx.moveTo(x, MARKER_TRACK_HEIGHT - 0);
			ctx.lineTo(x, MARKER_TRACK_HEIGHT - 16);
			ctx.stroke();
		}

		var mul = tickMark3 / tickMark2;
		units = time_scale / tickMark3;
		count = (width - LEFT_GUTTER + offsetUnits) / units;

		// small ticks
		for (i = 0; i < count; i++) {
			if (i % mul === 0) continue;
			ctx.strokeStyle = Theme.c;
			ctx.beginPath();
			x = i * units + LEFT_GUTTER - offsetUnits;
			ctx.moveTo(x, MARKER_TRACK_HEIGHT - 0);
			ctx.lineTo(x, MARKER_TRACK_HEIGHT - 10);
			ctx.stroke();
		}

		// Encapsulate a scroll rect for the layers
		ctx_wrap
			.save()
			.translate(0, MARKER_TRACK_HEIGHT)
			.beginPath()
			.rect(0, 0, LayoutConstants.width, SCROLL_HEIGHT)
			.translate(-scrollLeft, -scrollTop)
			.clip()
			.run(drawLayerContents)
			.restore();

		// Current Marker / Cursor
		ctx.strokeStyle = 'red'; // Theme.c
		x = (currentTime - frame_start) * time_scale + LEFT_GUTTER;

		var txt = utils.format_friendly_seconds(currentTime);
		var textWidth = ctx.measureText(txt).width;

		var base_line = MARKER_TRACK_HEIGHT - 5, half_rect = textWidth / 2 + 4;

		ctx.beginPath();
		ctx.moveTo(x, base_line);
		ctx.lineTo(x, height);
		ctx.stroke();

		ctx.fillStyle = 'red'; // black
		ctx.textAlign = 'center';
		ctx.beginPath();
		ctx.moveTo(x, base_line + 5);
		ctx.lineTo(x + 5, base_line);
		ctx.lineTo(x + half_rect, base_line);
		ctx.lineTo(x + half_rect, base_line - 14);
		ctx.lineTo(x - half_rect, base_line - 14);
		ctx.lineTo(x - half_rect, base_line);
		ctx.lineTo(x - 5, base_line);
		ctx.closePath();
		ctx.fill();

		ctx.fillStyle = 'white';
		ctx.fillText(txt, x, base_line - 4);

		ctx.restore();

		needsRepaint = false;
		// pointerEvents();

	}

	function y_to_track(y) {
		if (y - MARKER_TRACK_HEIGHT < 0) return -1;
		return (y - MARKER_TRACK_HEIGHT + scrollTop) / LINE_HEIGHT | 0;
	}


	function x_to_time(x) {
		var units = time_scale / tickMark3;

		// return frame_start + (x - LEFT_GUTTER) / time_scale;

		return frame_start + ((x - LEFT_GUTTER) / units | 0) / tickMark3;
	}

	function time_to_x(s) {
		var ds = s - frame_start;
		ds *= time_scale;
		ds += LEFT_GUTTER;

		return ds;
	}
	this.repaint = repaint;
	this._paint = _paint;

	repaint();
	var canvasBounds;

	document.addEventListener('mousemove', onMouseMove);

	track_canvas.addEventListener('dblclick', function(e) {
		canvasBounds = track_canvas.getBoundingClientRect();
		e.clientX - canvasBounds.left ; var my = e.clientY - canvasBounds.top;


		var track = y_to_track(my);


		dispatcher.fire('keyframe', layers[track], currentTime);

	});

	function onMouseMove(e) {
		canvasBounds = track_canvas.getBoundingClientRect();
		var mx = e.clientX - canvasBounds.left , my = e.clientY - canvasBounds.top;
		onPointerMove(mx, my);
	}
	var pointer = null;

	function onPointerMove(x, y) {
		if (mousedownItem) return;
		pointer = { x: x, y: y };
	}

	track_canvas.addEventListener('mouseout', function() {
		pointer = null;
	});

	var mousedown2 = false, mouseDownThenMove = false;
	handleDrag(track_canvas, function down(e) {
		mousedown2 = true;
		pointer = {
			x: e.offsetx,
			y: e.offsety
		};
		pointerEvents();

		if (!mousedownItem) dispatcher.fire('time.update', x_to_time(e.offsetx));
		// Hit criteria
	}, function move(e) {
		mousedown2 = false;
		if (mousedownItem) {
			mouseDownThenMove = true;
			if (mousedownItem.mousedrag) {
				mousedownItem.mousedrag(e);
			}
		} else {
			dispatcher.fire('time.update', x_to_time(e.offsetx));
		}
	}, function up(e) {
		if (mouseDownThenMove) {
			dispatcher.fire('keyframe.move');
		}
		else {
			dispatcher.fire('time.update', x_to_time(e.offsetx));
		}
		mousedown2 = false;
		mousedownItem = null;
		mouseDownThenMove = false;
	}
	);

	this.setState = function(state) {
		layers = state.value;
		repaint();
	};

}

// ********** class: ScrollBar ****************** //
/*
	Simple UI widget that displays a scrolltrack
	and slider, that fires some scroll events
*/
// ***********************************************

var scrolltrack_style = {
	// float: 'right',
	position: 'absolute',
	// right: '0',
	// top: '0',
	// bottom: '0',
	background: '-webkit-gradient(linear, left top, right top, color-stop(0, rgb(29,29,29)), color-stop(0.6, rgb(50,50,50)) )',
	border: '1px solid rgb(29, 29, 29)',
	// zIndex: '1000',
	textAlign: 'center',
	cursor: 'pointer'
};

var scrollbar_style = {
	background: '-webkit-gradient(linear, left top, right top, color-stop(0.2, rgb(88,88,88)), color-stop(0.6, rgb(64,64,64)) )',
	border: '1px solid rgb(25,25,25)',
	// position: 'absolute',
	position: 'relative',
	borderRadius: '6px'
};

function ScrollBar(h, w, dispatcher) {

	var SCROLLBAR_WIDTH = w ? w : 12;
	var SCROLLBAR_MARGIN = 3;
	var SCROLL_WIDTH = SCROLLBAR_WIDTH + SCROLLBAR_MARGIN * 2;
	var MIN_BAR_LENGTH = 25;

	var scrolltrack = document.createElement('div');
	utils.style(scrolltrack, scrolltrack_style);

	var scrolltrackHeight = h - 2;
	scrolltrack.style.height = scrolltrackHeight + 'px';
	scrolltrack.style.width = SCROLL_WIDTH + 'px';

	// var scrollTop = 0;
	var scrollbar = document.createElement('div');
	// scrollbar.className = 'scrollbar';
	utils.style(scrollbar, scrollbar_style);
	scrollbar.style.width = SCROLLBAR_WIDTH + 'px';
	scrollbar.style.height = h / 2;
	scrollbar.style.top = 0;
	scrollbar.style.left = SCROLLBAR_MARGIN + 'px'; // 0; //S

	scrolltrack.appendChild(scrollbar);

	var me = this;

	var bar_length, bar_y;

	// Sets lengths of scrollbar by percentage
	this.setLength = function(l) {
		// limit 0..1
		l = Math.max(Math.min(1, l), 0);
		l *= scrolltrackHeight;
		bar_length = Math.max(l, MIN_BAR_LENGTH);
		scrollbar.style.height = bar_length + 'px';
	};

	this.setHeight = function(height) {
		h = height;

		scrolltrackHeight = h - 2;
		scrolltrack.style.height = scrolltrackHeight + 'px' ;
	};

	// Moves scrollbar to position by Percentage
	this.setPosition = function(p) {
		p = Math.max(Math.min(1, p), 0);
		var emptyTrack = scrolltrackHeight - bar_length;
		bar_y = p * emptyTrack;
		scrollbar.style.top = bar_y + 'px';
	};

	this.setLength(1);
	this.setPosition(0);
	this.onScroll = new Do();

	var mouse_down_grip;

	function onDown(event) {
		event.preventDefault();

		if (event.target == scrollbar) {
			mouse_down_grip = event.clientY;
			document.addEventListener('mousemove', onMove, false);
			document.addEventListener('mouseup', onUp, false);
		} else {
			if (event.clientY < bar_y) {
				me.onScroll.fire('pageup');
			} else if (event.clientY > (bar_y + bar_length)) {
				me.onScroll.fire('pagedown');
			}
			// if want to drag scroller to empty track instead
			// me.setPosition(event.clientY / (scrolltrackHeight - 1));
		}
	}

	function onMove(event) {
		event.preventDefault();

		// event.target == scrollbar
		var emptyTrack = scrolltrackHeight - bar_length;
		var scrollto = (event.clientY - mouse_down_grip) / emptyTrack;

		// clamp limits to 0..1
		if (scrollto > 1) scrollto = 1;
		if (scrollto < 0) scrollto = 0;
		me.setPosition(scrollto);
		me.onScroll.fire('scrollto', scrollto);
	}

	function onUp(event) {
		onMove(event);
		document.removeEventListener('mousemove', onMove, false);
		document.removeEventListener('mouseup', onUp, false);
	}

	scrolltrack.addEventListener('mousedown', onDown, false);
	this.dom = scrolltrack;

}

var package_json = { version: "test-version" };

// Data Store with a source of truth
function DataStore() {
	this.DELIMITER = ':';
	this.blank();
	this.onOpen = new Do();
	this.onSave = new Do();

	this.listeners = [];
}

DataStore.prototype.addListener = function(path, cb) {
	this.listeners.push({
		path: path,
		callback: cb
	});
};

DataStore.prototype.blank = function() {
	var data = {};

	data.version = package_json.version;
	data.modified = new Date().toString();
	data.title = 'Untitled';

	data.ui = {
		currentTime: 0,
		totalTime: LayoutConstants.default_length,
		scrollTime: 0,
		timeScale: LayoutConstants.time_scale
	};

	data.layers = [];

	this.data = data;
};

DataStore.prototype.update = function() {
	var data = this.data;

	data.version = package_json.version;
	data.modified = new Date().toString();
};

DataStore.prototype.setJSONString = function(data) {
	this.data = JSON.parse(data);
};

DataStore.prototype.setJSON = function(data) {
	this.data = data;
};

DataStore.prototype.getJSONString = function(format) {
	return JSON.stringify(this.data, null, format);
};

DataStore.prototype.getValue = function(paths) {
	var descend = paths.split(this.DELIMITER);
	var reference = this.data;
	for (var i = 0, il = descend.length; i < il; i++) {
		var path = descend[i];
		if (reference[path] === undefined) {
			console.warn('Cant find ' + paths);
			return;
		}
		reference = reference[path];
	}
	return reference;
};

DataStore.prototype.setValue = function(paths, value) {
	var descend = paths.split(this.DELIMITER);
	var reference = this.data;
	var path;
	for (var i = 0, il = descend.length - 1; path = descend[i], i < il ; i++) {
		reference = reference[path];
	}

	reference[path] = value;

	this.listeners.forEach(function(l) {
		if (paths.indexOf(l.path) > -1) l.callback();
	});
};

DataStore.prototype.get = function(path, suffix) {
	if (suffix) path = suffix + this.DELIMITER + path;
	return new DataProx(this, path);
};

function DataProx(store, path) {
	this.path = path;
	this.store = store;
}

DataProx.prototype = {
	get value() {
		return this.store.getValue(this.path);
	},
	set value(val) {
		this.store.setValue(this.path, val);
	}
};

DataProx.prototype.get = function(path) {
	return this.store.get(path, this.path);
};

const SNAP_FULL_SCREEN = 'full-screen';
const SNAP_TOP_EDGE = 'snap-top-edge'; // or actually top half
const SNAP_LEFT_EDGE = 'snap-left-edge';
const SNAP_RIGHT_EDGE = 'snap-right-edge';
const SNAP_BOTTOM_EDGE = 'snap-bottom-edge';
const SNAP_DOCK_BOTTOM = 'dock-bottom';

function setBounds(element, x, y, w, h) {
	element.style.left = x + 'px';
	element.style.top = y + 'px';
	element.style.width = w + 'px';
	element.style.height = h + 'px';
}

/*

The Docking Widget

1. when .allowMove(true) is set, the pane becomes draggable
2. when dragging, if the pointer to near to the edges,
   it resizes the ghost pannel as a suggestion to snap into the
   suggested position
3. user can either move pointer away or let go of the cursor,
   allow the pane to be resized and snapped into position


My origin implementation from https://codepen.io/zz85/pen/gbOoVP

args eg.
	var pane = document.getElementById('pane');
	var ghostpane = document.getElementById('ghostpane');
	widget = new DockingWindow(pane, ghostpane)


dom.addEventListener('mouseover', function() {
		widget.allowMove(true);
	});

	title_dom.addEventListener('mouseout', function() {
		widget.allowMove(false);
	});

	resize_full.onClick(() => {
		widget.maximize() // fill to screen
	})

	// TODO callback when pane is resized
	widget.resizes.do(() => {
		something
	})
*/

function DockingWindow(pane, ghostpane) {

	// Minimum resizable area
	var minWidth = 100;
	var minHeight = 80;

	// Thresholds
	var FULLSCREEN_MARGINS = 2;
	var SNAP_MARGINS = 8;
	var MARGINS = 2;

	// End of what's configurable.
	var pointerStart = null;
	var onRightEdge, onBottomEdge, onLeftEdge, onTopEdge;

	var preSnapped;

	var bounds, x, y;

	var redraw = false;

	var allowDragging = true;
	var snapType;

	this.allowMove = function(allow) {
		allowDragging = allow;
	};

	function canMove() {
		return allowDragging;
	}

	this.maximize = function() {
		if (!preSnapped) {
			preSnapped = {
				width: bounds.width,
				height: bounds.height,
				top: bounds.top,
				left: bounds.left,
			};

			snapType = SNAP_FULL_SCREEN;
			resizeEdges();
		} else {
			setBounds(pane, bounds.left, bounds.top, bounds.width, bounds.height);
			calculateBounds();
			snapType = null;
			preSnapped = null;
		}
	};

	this.resizes = new Do();

	/* DOM Utils */
	function hideGhostPane() {
		// hide the hinter, animatating to the pane's bounds
		setBounds(ghostpane, bounds.left, bounds.top, bounds.width, bounds.height);
		ghostpane.style.opacity = 0;
	}

	function onTouchDown(e) {
		onDown(e.touches[0]);
		e.preventDefault();
	}

	function onTouchMove(e) {
		onMove(e.touches[0]);
	}

	function onTouchEnd(e) {
		if (e.touches.length == 0) onUp(e.changedTouches[0]);
	}

	function onMouseDown(e) {
		onDown(e);
	}

	function onMouseUp(e) {
		onUp(e);
	}

	function onDown(e) {
		calculateBounds(e);

		var isResizing = onRightEdge || onBottomEdge || onTopEdge || onLeftEdge;
		var isMoving = !isResizing && canMove();

		pointerStart = {
			x: x,
			y: y,
			cx: e.clientX,
			cy: e.clientY,
			w: bounds.width,
			h: bounds.height,
			isResizing: isResizing,
			isMoving: isMoving,
			onTopEdge: onTopEdge,
			onLeftEdge: onLeftEdge,
			onRightEdge: onRightEdge,
			onBottomEdge: onBottomEdge
		};

		if (isResizing || isMoving) {
			e.preventDefault();
			e.stopPropagation();
		}
	}


	function calculateBounds(e) {
		bounds = pane.getBoundingClientRect();
		x = e.clientX - bounds.left;
		y = e.clientY - bounds.top;

		onTopEdge = y < MARGINS;
		onLeftEdge = x < MARGINS;
		onRightEdge = x >= bounds.width - MARGINS;
		onBottomEdge = y >= bounds.height - MARGINS;
	}

	var e; // current mousemove event

	function onMove(ee) {
		e = ee;
		calculateBounds(e);

		redraw = true;
	}

	function animate() {

		requestAnimationFrame(animate);

		if (!redraw) return;

		redraw = false;

		// style cursor
		if (onRightEdge && onBottomEdge || onLeftEdge && onTopEdge) {
			pane.style.cursor = 'nwse-resize';
		} else if (onRightEdge && onTopEdge || onBottomEdge && onLeftEdge) {
			pane.style.cursor = 'nesw-resize';
		} else if (onRightEdge || onLeftEdge) {
			pane.style.cursor = 'ew-resize';
		} else if (onBottomEdge || onTopEdge) {
			pane.style.cursor = 'ns-resize';
		} else if (canMove()) {
			pane.style.cursor = 'move';
		} else {
			pane.style.cursor = 'default';
		}

		if (!pointerStart) return;

		/* User is resizing */
		if (pointerStart.isResizing) {

			if (pointerStart.onRightEdge) pane.style.width = Math.max(x, minWidth) + 'px';
			if (pointerStart.onBottomEdge) pane.style.height = Math.max(y, minHeight) + 'px';

			if (pointerStart.onLeftEdge) {
				var currentWidth = Math.max(pointerStart.cx - e.clientX  + pointerStart.w, minWidth);
				if (currentWidth > minWidth) {
					pane.style.width = currentWidth + 'px';
					pane.style.left = e.clientX + 'px';
				}
			}

			if (pointerStart.onTopEdge) {
				var currentHeight = Math.max(pointerStart.cy - e.clientY  + pointerStart.h, minHeight);
				if (currentHeight > minHeight) {
					pane.style.height = currentHeight + 'px';
					pane.style.top = e.clientY + 'px';
				}
			}

			hideGhostPane();

			self.resizes.fire(bounds.width, bounds.height);

			return;
		}

		/* User is dragging */
		if (pointerStart.isMoving) {
			var snapType = checkSnapType();
			if (snapType) {
				calcSnapBounds(snapType);
				// console.log('snapping...', JSON.stringify(snapBounds))
				var { left, top, width, height } = snapBounds;
				setBounds(ghostpane, left, top, width, height);
				ghostpane.style.opacity = 0.2;
			} else {
				hideGhostPane();
			}

			if (preSnapped) {
				setBounds(pane,
					e.clientX - preSnapped.width / 2,
					e.clientY - Math.min(pointerStart.y, preSnapped.height),
					preSnapped.width,
					preSnapped.height
				);
				return;
			}

			// moving
			pane.style.top = (e.clientY - pointerStart.y) + 'px';
			pane.style.left = (e.clientX - pointerStart.x) + 'px';

			return;
		}
	}

	function checkSnapType() {
		// drag to full screen
		if (e.clientY < FULLSCREEN_MARGINS) return SNAP_FULL_SCREEN;

		// drag for top half screen
		if (e.clientY < SNAP_MARGINS) return SNAP_TOP_EDGE;

		// drag for left half screen
		if (e.clientX < SNAP_MARGINS) return SNAP_LEFT_EDGE;

		// drag for right half screen
		if (window.innerWidth - e.clientX < SNAP_MARGINS) return SNAP_RIGHT_EDGE;

		// drag for bottom half screen
		if (window.innerHeight - e.clientY < SNAP_MARGINS) return SNAP_BOTTOM_EDGE;

	}

	var self = this;

	var snapBounds = {};

	function calcSnapBounds(snapType) {
		if (!snapType) return;

		var width, height, left, top;

		switch (snapType) {
		case SNAP_FULL_SCREEN:
			width = window.innerWidth;
			left = 0;
			top = 48;
			height = window.innerHeight - top;
			break;
		case SNAP_TOP_EDGE:
			width = window.innerWidth;
			height = window.innerHeight / 2;
			left = 0;
			top = 0;
			break;
		case SNAP_LEFT_EDGE:
			width = window.innerWidth / 2;
			height = window.innerHeight;
			left = 0;
			top = 0;
			break;
		case SNAP_RIGHT_EDGE:
			width = window.innerWidth / 2;
			height = window.innerHeight;
			left = window.innerWidth - width;
			top = 0;
			break;
		case SNAP_BOTTOM_EDGE:
			width = window.innerWidth;
			height = window.innerHeight / 3;
			left = 0;
			top = window.innerHeight - height;
			break;
		case SNAP_DOCK_BOTTOM:
			width = bounds.width;
			height = bounds.height;
			left = (window.innerWidth - width) * 0.5;
			top = window.innerHeight - height;
		}

		Object.assign(snapBounds, { left, top, width, height });
	}

	/* When one of the edges is move, resize pane */
	function resizeEdges() {
		if (!snapType) return;

		calcSnapBounds(snapType);
		var { left, top, width, height } = snapBounds;
		setBounds(pane, left, top, width, height);

		self.resizes.fire(width, height);
	}

	function onUp(e) {
		calculateBounds(e);

		if (pointerStart && pointerStart.isMoving) {
			// Snap
			snapType = checkSnapType();
			if (snapType) {
				preSnapped = {
					width: bounds.width,
					height: bounds.height,
					top: bounds.top,
					left: bounds.left,
				};
				resizeEdges();
			} else {
				preSnapped = null;
			}

			hideGhostPane();
		}

		pointerStart = null;
	}

	function init() {
		window.addEventListener('resize', function() {
			resizeEdges();
		});

		setBounds(pane, 0, 0, LayoutConstants.width, LayoutConstants.height);
		setBounds(ghostpane, 0, 0, LayoutConstants.width, LayoutConstants.height);

		// Mouse events
		pane.addEventListener('mousedown', onMouseDown);
		document.addEventListener('mousemove', onMove);
		document.addEventListener('mouseup', onMouseUp);

		// Touch events
		pane.addEventListener('touchstart', onTouchDown);
		document.addEventListener('touchmove', onTouchMove);
		document.addEventListener('touchend', onTouchEnd);

		bounds = pane.getBoundingClientRect();
		snapType = SNAP_FULL_SCREEN;

		// use setTimeout as a hack to get diemensions correctly! :(
		setTimeout(() => resizeEdges());
		hideGhostPane();

		animate();
	}

	init();
}

/*
 * @author Joshua Koo http://joshuakoo.com
 */

const TIMELINER_VERSION = "2.0.0-dev";
var style = utils.style;
var saveToFile = utils.saveToFile;
var openAs = utils.openAs;
var STORAGE_PREFIX = utils.STORAGE_PREFIX;

var Z_INDEX = 0;

function LayerProp(name) {
	this.name = name;
	this.values = [];

	this._value = 0;

	this._color = '#' + (Math.random() * 0xffffff | 0).toString(16);
	/*
	this.max
	this.min
	this.step
	*/
}

function Timeliner(target) {
	// Dispatcher for coordination
	var dispatcher = new Dispatcher();

	// Data
	var data = new DataStore();
	var layer_store = data.get('layers');
	var layers = layer_store.value;

	window._data = data; // expose it for debugging

	// Undo manager
	var undo_manager = new UndoManager(dispatcher);

	// Views
	var timeline = new TimelinePanel(data, dispatcher);
	var layer_panel = new LayerCabinet(data, dispatcher);

	setTimeout(function() {
		// hack!
		undo_manager.save(new UndoState(data, 'Loaded'), true);
	});

	dispatcher.on('keyframe', function(layer, value) {
		layers.indexOf(layer);

		var t = data.get('ui:currentTime').value;
		var v = utils.findTimeinLayer(layer, t);

		// console.log(v, '...keyframe index', index, utils.format_friendly_seconds(t), typeof(v));
		// console.log('layer', layer, value);

		if (typeof(v) === 'number') {
			layer.values.splice(v, 0, {
				time: t,
				value: value,
				_color: '#' + (Math.random() * 0xffffff | 0).toString(16)
			});

			undo_manager.save(new UndoState(data, 'Add Keyframe'));
		} else {
			console.log('remove from index', v);
			layer.values.splice(v.index, 1);

			undo_manager.save(new UndoState(data, 'Remove Keyframe'));
		}

		repaintAll();

	});

	dispatcher.on('keyframe.move', function(layer, value) {
		undo_manager.save(new UndoState(data, 'Move Keyframe'));
	});

	// dispatcher.fire('value.change', layer, me.value);
	dispatcher.on('value.change', function(layer, value, dont_save) {
		if (layer._mute) return;

		var t = data.get('ui:currentTime').value;
		var v = utils.findTimeinLayer(layer, t);

		// console.log(v, 'value.change', layer, value, utils.format_friendly_seconds(t), typeof(v));
		if (typeof(v) === 'number') {
			layer.values.splice(v, 0, {
				time: t,
				value: value,
				_color: '#' + (Math.random() * 0xffffff | 0).toString(16)
			});
			if (!dont_save) undo_manager.save(new UndoState(data, 'Add value'));
		} else {
			v.object.value = value;
			if (!dont_save) undo_manager.save(new UndoState(data, 'Update value'));
		}

		repaintAll();
	});

	dispatcher.on('action:solo', function(layer, solo) {
		layer._solo = solo;

		console.log(layer, solo);

		// When a track is solo-ed, playback only changes values
		// of that layer.
	});

	dispatcher.on('action:mute', function(layer, mute) {
		layer._mute = mute;

		// When a track is mute, playback does not play
		// frames of those muted layers.

		// also feels like hidden feature in photoshop

		// when values are updated, eg. from slider,
		// no tweens will be created.
		// we can decide also to "lock in" layers
		// no changes to tween will be made etc.
	});

	dispatcher.on('ease', function(layer, ease_type) {
		var t = data.get('ui:currentTime').value;
		var v = utils.timeAtLayer(layer, t);
		// console.log('Ease Change > ', layer, value, v);
		if (v && v.entry) {
			v.entry.tween  = ease_type;
		}

		undo_manager.save(new UndoState(data, 'Add Ease'));

		repaintAll();
	});

	var start_play = null,
		played_from = 0; // requires some more tweaking

	dispatcher.on('controls.toggle_play', function() {
		if (start_play) {
			pausePlaying();
		} else {
			startPlaying();
		}
	});

	dispatcher.on('controls.restart_play', function() {
		if (!start_play) {
			startPlaying();
		}

		setCurrentTime(played_from);
	});

	dispatcher.on('controls.play', startPlaying);
	dispatcher.on('controls.pause', pausePlaying);

	function startPlaying() {
		play_button();
		// played_from = timeline.current_frame;
		start_play = performance.now() - data.get('ui:currentTime').value * 1000;
		layer_panel.setControlStatus(true);
		// dispatcher.fire('controls.status', true);
	}

	function pausePlaying() {
		stop_button();
		start_play = null;
		layer_panel.setControlStatus(false);
		// dispatcher.fire('controls.status', false);
	}

	dispatcher.on('controls.stop', function() {
		if (start_play !== null) pausePlaying();
		setCurrentTime(0);
	});

	var currentTimeStore = data.get('ui:currentTime');
	dispatcher.on('time.update', setCurrentTime);

	dispatcher.on('totalTime.update', function(value) {
		// context.totalTime = value;
		// controller.setDuration(value);
		// timeline.repaint();
	});

	/* update scroll viewport */
	dispatcher.on('update.scrollTime', function(v) {
		v = Math.max(0, v);
		data.get('ui:scrollTime').value = v;
		repaintAll();
	});


	function setCurrentTime(value) {
		value = Math.max(0, value);
		currentTimeStore.value = value;

		if (start_play) start_play = performance.now() - value * 1000;
		repaintAll();
		// layer_panel.repaint(s);
	}

	dispatcher.on('target.notify', function(name, value) {
		if (target) target[name] = value;
	});

	dispatcher.on('update.scale', function(v) {
		console.log('range', v);
		data.get('ui:timeScale').value = v;

		timeline.repaint();
	});

	// handle undo / redo
	dispatcher.on('controls.undo', function() {
		var history = undo_manager.undo();
		data.setJSONString(history.state);

		updateState();
	});

	dispatcher.on('controls.redo', function() {
		var history = undo_manager.redo();
		data.setJSONString(history.state);

		updateState();
	});

	/*
		Paint Routines
	*/

	function paint() {
		requestAnimationFrame(paint);

		if (start_play) {
			var t = (performance.now() - start_play) / 1000;
			setCurrentTime(t);


			if (t > data.get('ui:totalTime').value) {
				// simple loop
				start_play = performance.now();
			}
		}

		if (needsResize) {
			div.style.width = LayoutConstants.width + 'px';
			div.style.height = LayoutConstants.height + 'px';

			restyle(layer_panel.dom, timeline.dom);

			timeline.resize();
			repaintAll();
			needsResize = false;

			dispatcher.fire('resize');
		}

		timeline._paint();
	}

	paint();

	/*
		End Paint Routines
	*/

	function save(name) {
		if (!name) name = 'autosave';

		var json = data.getJSONString();

		try {
			localStorage[STORAGE_PREFIX + name] = json;
			dispatcher.fire('save:done');
		} catch (e) {
			console.log('Cannot save', name, json);
		}
	}

	function saveAs(name) {
		if (!name) name = data.get('name').value;
		name = prompt('Pick a name to save to (localStorage)', name);
		if (name) {
			data.data.name = name;
			save(name);
		}
	}

	function saveSimply() {
		var name = data.get('name').value;
		if (name) {
			save(name);
		} else {
			saveAs(name);
		}
	}

	function exportJSON() {
		var json = data.getJSONString();
		var ret = prompt('Hit OK to download otherwise Copy and Paste JSON', json);

		console.log(JSON.stringify(data.data, null, '\t'));
		if (!ret) return;

		// make json downloadable
		json = data.getJSONString('\t');
		var fileName = 'timeliner-test' + '.json';

		saveToFile(json, fileName);
	}

	function loadJSONString(o) {
		// should catch and check errors here
		var json = JSON.parse(o);
		load(json);
	}

	function load(o) {
		data.setJSON(o);
		//
		if (data.getValue('ui') === undefined) {
			data.setValue('ui', {
				currentTime: 0,
				totalTime: LayoutConstants.default_length,
				scrollTime: 0,
				timeScale: LayoutConstants.time_scale
			});
		}

		undo_manager.clear();
		undo_manager.save(new UndoState(data, 'Loaded'), true);

		updateState();
	}

	function updateState() {
		layers = layer_store.value; // FIXME: support Arrays
		layer_panel.setState(layer_store);
		timeline.setState(layer_store);

		repaintAll();
	}

	function repaintAll() {
		var content_height = layers.length * LayoutConstants.LINE_HEIGHT;
		scrollbar.setLength(LayoutConstants.TIMELINE_SCROLL_HEIGHT / content_height);

		layer_panel.repaint();
		timeline.repaint();
	}

	function promptImport() {
		var json = prompt('Paste JSON in here to Load');
		if (!json) return;
		console.log('Loading.. ', json);
		loadJSONString(json);
	}

	function open(title) {
		if (title) {
			loadJSONString(localStorage[STORAGE_PREFIX + title]);
		}
	}

	this.openLocalSave = open;

	dispatcher.on('import', function() {
		promptImport();
	}.bind(this));

	dispatcher.on('new', function() {
		data.blank();
		updateState();
	});

	dispatcher.on('openfile', function() {
		openAs(function(data) {
			// console.log('loaded ' + data);
			loadJSONString(data);
		}, div);
	});

	dispatcher.on('open', open);
	dispatcher.on('export', exportJSON);

	dispatcher.on('save', saveSimply);
	dispatcher.on('save_as', saveAs);

	// Expose API
	this.save = save;
	this.load = load;

	/*
		Start DOM Stuff (should separate file)
	*/

	var div = document.createElement('div');
	style(div, {
		textAlign: 'left',
		lineHeight: '1em',
		position: 'absolute',
		top: '22px'
	});

	var pane = document.createElement('div');

	style(pane, {
		position: 'fixed',
		top: '20px',
		left: '20px',
		margin: 0,
		border: '1px solid ' + Theme.a,
		padding: 0,
		overflow: 'hidden',
		backgroundColor: Theme.a,
		color: Theme.d,
		zIndex: Z_INDEX,
		fontFamily: 'monospace',
		fontSize: '12px'
	});


	var header_styles = {
		position: 'absolute',
		top: '0px',
		width: '100%',
		height: '22px',
		lineHeight: '22px',
		overflow: 'hidden'
	};

	var button_styles = {
		width: '20px',
		height: '20px',
		padding: '2px',
		marginRight: '2px'
	};

	// var pane_title = document.createElement('div');
	// style(pane_title, header_styles, {
	// 	borderBottom: '1px solid ' + Theme.b,
	// 	textAlign: 'center'
	// });

	// var title_bar = document.createElement('span');
	// pane_title.appendChild(title_bar);

	// title_bar.innerHTML = 'Timeliner ' + TIMELINER_VERSION;
	// pane_title.appendChild(title_bar);

	// var top_right_bar = document.createElement('div');
	// style(top_right_bar, header_styles, {
	// 	textAlign: 'right'
	// });

	// pane_title.appendChild(top_right_bar);

	// resize minimize
	// var resize_small = new IconButton(10, 'resize_small', 'minimize', dispatcher);
	// top_right_bar.appendChild(resize_small.dom);

	// resize full
	// var resize_full = new IconButton(10, 'resize_full', 'maximize', dispatcher);
	// style(resize_full.dom, button_styles, { marginRight: '2px' });
	// top_right_bar.appendChild(resize_full.dom);

	var pane_status = document.createElement('div');

	var footer_styles = {
		position: 'absolute',
		width: '100%',
		height: '22px',
		lineHeight: '22px',
		bottom: '0',
		// padding: '2px',
		background: Theme.a,
		fontSize: '11px'
	};

	style(pane_status, footer_styles, {
		borderTop: '1px solid ' + Theme.b,
	});

	pane.appendChild(div);
	pane.appendChild(pane_status);
	// pane.appendChild(pane_title);

	var label_status = document.createElement('span');
	label_status.textContent = 'hello!';
	label_status.style.marginLeft = '10px';

	this.setStatus = function(text) {
		label_status.textContent = text;
	};

	dispatcher.on('state:save', function(description) {
		dispatcher.fire('status', description);
		save('autosave');
	});

	dispatcher.on('status', this.setStatus);

	var bottom_right = document.createElement('div');
	style(bottom_right, footer_styles, {
		textAlign: 'right'
	});


	// var button_save = document.createElement('button');
	// style(button_save, button_styles);
	// button_save.textContent = 'Save';
	// button_save.onclick = function() {
	// 	save();
	// };

	// var button_load = document.createElement('button');
	// style(button_load, button_styles);
	// button_load.textContent = 'Import';
	// button_load.onclick = this.promptLoad;

	// var button_open = document.createElement('button');
	// style(button_open, button_styles);
	// button_open.textContent = 'Open';
	// button_open.onclick = this.promptOpen;


	// bottom_right.appendChild(button_load);
	// bottom_right.appendChild(button_save);
	// bottom_right.appendChild(button_open);

	pane_status.appendChild(label_status);
	pane_status.appendChild(bottom_right);


	/**/
	// zoom in
	// new IconButton(12, 'zoom_in', 'zoom in', dispatcher);
	// // zoom out
	// new IconButton(12, 'zoom_out', 'zoom out', dispatcher);
	// // settings
	// new IconButton(12, 'cog', 'settings', dispatcher);

	// bottom_right.appendChild(zoom_in.dom);
	// bottom_right.appendChild(zoom_out.dom);
	// bottom_right.appendChild(cog.dom);

	// // add layer
	// var plus = new IconButton(12, 'plus', 'New Layer', dispatcher);
	// plus.onClick(function() {
	// 	var name = prompt('Layer name?');
	// 	addLayer(name);

	// 	undo_manager.save(new UndoState(data, 'Layer added'));

	// 	repaintAll();
	// });
	// style(plus.dom, button_styles);
	// bottom_right.appendChild(plus.dom);


	// // trash
	// var trash = new IconButton(12, 'trash', 'Delete save', dispatcher);
	// trash.onClick(function() {
	// 	var name = data.get('name').value;
	// 	if (name && localStorage[STORAGE_PREFIX + name]) {
	// 		var ok = confirm('Are you sure you wish to delete ' + name + '?');
	// 		if (ok) {
	// 			delete localStorage[STORAGE_PREFIX + name];
	// 			dispatcher.fire('status', name + ' deleted');
	// 			dispatcher.fire('save:done');
	// 		}
	// 	}
	// });
	// style(trash.dom, button_styles, { marginRight: '2px' });
	// bottom_right.appendChild(trash.dom);


	// pane_status.appendChild(document.createTextNode(' | TODO <Dock Full | Dock Botton | Snap Window Edges | zoom in | zoom out | Settings | help>'));

	/*
			End DOM Stuff
	*/

	var ghostpane = document.createElement('div');
	ghostpane.id = 'ghostpane';
	style(ghostpane, {
		// background: '#999',
		opacity: 0.2,
		position: 'fixed',
		margin: 0,
		padding: 0,
		zIndex: (Z_INDEX - 1),
		// transition: 'all 0.25s ease-in-out',
		transitionProperty: 'top, left, width, height, opacity',
		transitionDuration: '0.25s',
		transitionTimingFunction: 'ease-in-out'
	});


	//
	// Handle DOM Views
	//

	// Shadow Root
	var root = document.createElement('timeliner');
	// document.body.appendChild(root);
	document.body.insertBefore(root, document.body.firstElementChild);
	// if (root.createShadowRoot) root = root.createShadowRoot();

	//window.r = root;

	// var iframe = document.createElement('iframe');
	// document.body.appendChild(iframe);
	// root = iframe.contentDocument.body;

	root.appendChild(pane);
	root.appendChild(ghostpane);

	div.appendChild(layer_panel.dom);
	div.appendChild(timeline.dom);

	var scrollbar = new ScrollBar(200, 10);
	div.appendChild(scrollbar.dom);

	// percentages
	scrollbar.onScroll.do(function(type, scrollTo) {
		switch (type) {
		case 'scrollto':
			layer_panel.scrollTo(scrollTo);
			timeline.scrollTo(scrollTo);
			break;
	//		case 'pageup':
	// 			scrollTop -= pageOffset;
	// 			me.draw();
	// 			me.updateScrollbar();
	// 			break;
	// 		case 'pagedown':
	// 			scrollTop += pageOffset;
	// 			me.draw();
	// 			me.updateScrollbar();
	// 			break;
		}
	});

	function hide() {
		root.style.display = "none";
	}
	this.hide = hide;

	function show() {
		root.style.display = "block";
	}
	this.show = show;

	// document.addEventListener('keypress', function(e) {
	// 	console.log('kp', e);
	// });
	// document.addEventListener('keyup', function(e) {
	// 	if (undo) console.log('UNDO');

	// 	console.log('kd', e);
	// });

	// TODO: Keyboard Shortcuts
	// Esc - Stop and review to last played from / to the start?
	// Space - play / pause from current position
	// Enter - play all
	// k - keyframe

	// document.addEventListener('keydown', function(e) {
	// 	if (event.isComposing || event.keyCode === 229) {
	//     return;
  // 	}
	// 	var play = e.keyCode == 32; // space
	// 	var enter = e.keyCode == 13; //
	// 	e.metaKey && e.keyCode == 91 && !e.shiftKey;

	// 	var active = document.activeElement;
	// 	// console.log( active.nodeName );

	// 	if (active.nodeName.match(/(INPUT|BUTTON|SELECT|TIMELINER)/)) {
	// 		active.blur();
	// 	}

	// 	if (play) {
	// 		dispatcher.fire('controls.toggle_play');
	// 	}
	// 	else if (enter) {
	// 		// FIXME: Return should play from the start or last played from?
	// 		dispatcher.fire('controls.restart_play');
	// 		// dispatcher.fire('controls.undo');
	// 	}
	// 	else if (e.keyCode == 27) {
	// 		// Esc = stop. FIXME: should rewind head to last played from or Last pointed from?
	// 		dispatcher.fire('controls.pause');
	// 	}
	// 	else console.log('keydown', e.keyCode);
	// });

	var needsResize = true;

	function resize(width, height) {
		// data.get('ui:bounds').value = {
		// 	width: width,
		// 	height: height
		// };
		// TODO: remove ugly hardcodes
		width -= 4;
		height -= 44;

		LayoutConstants.width = width - LayoutConstants.LEFT_PANE_WIDTH;
		LayoutConstants.height = height;

		LayoutConstants.TIMELINE_SCROLL_HEIGHT = height - LayoutConstants.MARKER_TRACK_HEIGHT;
		var scrollable_height = LayoutConstants.TIMELINE_SCROLL_HEIGHT;

		scrollbar.setHeight(scrollable_height - 2);

		style(scrollbar.dom, {
			top: LayoutConstants.MARKER_TRACK_HEIGHT + 'px',
			left: (width - 16) + 'px',
		});

		needsResize = true;
	}

	function restyle(left, right) {
		left.style.cssText = 'position: absolute; left: 0px; top: 0px; height: ' + LayoutConstants.height + 'px;';
		style(left, {
			// background: Theme.a,
			overflow: 'hidden'
		});
		left.style.width = LayoutConstants.LEFT_PANE_WIDTH + 'px';

		// right.style.cssText = 'position: absolute; top: 0px;';
		right.style.position = 'absolute';
		right.style.top = '0px';
		right.style.left = LayoutConstants.LEFT_PANE_WIDTH + 'px';
	}

	function addLayer(name) {
		var layer = new LayerProp(name);

		layers = layer_store.value;
		layers.push(layer);

		layer_panel.setState(layer_store);
	}

	this.addLayer = addLayer;

	this.dispose = function dispose() {

		var domParent = pane.parentElement;
		domParent.removeChild(pane);
		domParent.removeChild(ghostpane);

	};

	this.setTarget = function(t) {
		target = t;
	};

	function getValueRanges(ranges, interval) {
		interval = interval ? interval : 0.15;
		ranges = ranges ? ranges : 2;

		// not optimized!
		var t = data.get('ui:currentTime').value;

		var values = [];

		for (var u = -ranges; u <= ranges; u++) {
			// if (u == 0) continue;
			var o = {};

			for (var l = 0; l < layers.length; l++) {
				var layer = layers[l];
				var m = utils.timeAtLayer(layer, t + u * interval);
				o[layer.name] = m.value;
			}

			values.push(o);

		}

		return values;
	}

	this.getValues = getValueRanges;

	/* Integrate pane into docking window */
	var widget = new DockingWindow(pane, ghostpane);
	console.log('widget:',widget);
	widget.allowMove(false);
	widget.resizes.do(resize);

	// pane_title.addEventListener('mouseover', function() {
	// 	widget.allowMove(true);
	// });

	// pane_title.addEventListener('mouseout', function() {
	// 	widget.allowMove(false);
	// });
}

var target = {
	x: 0,
	y: 0,
	rotate: 0
};

// initialize timeliner
var timeliner = new Timeliner(target);
console.log(timeliner);
timeliner.hide();

// timeliner.load({
// 	"version":"1.2.0",
// 	"modified":"Mon Dec 08 2014 10:41:11 GMT+0800 (SGT)",
// 	"title":"Untitled",
// 	"layers":[{
// 			"name":"x",
// 			"values":[
// 				{"time":0.1,"value":0,"_color":"#893c0f","tween":"quadEaseIn"},
// 				{"time":3,"value":3.500023,"_color":"#b074a0"}
// 			],
// 			"tmpValue":3.500023,
// 			"_color":"#6ee167"
// 		},{
// 			"name":"y",
// 			"values":[
// 				{"time":0.1,"value":0,"_color":"#abac31","tween":"quadEaseOut"},
// 				{"time":0.5,"value":-1.000001,"_color":"#355ce8","tween":"quadEaseIn"},
// 				{"time":1.1,"value":0,"_color":"#47e90","tween":"quadEaseOut"},
// 				{"time":1.7,"value":-0.5,"_color":"#f76bca","tween":"quadEaseOut"},
// 				{"time":2.3,"value":0,"_color":"#d59cfd"}
// 			]
// 		},{
// 			"name":"rotate",
// 			"values":[
// 				{"time":0.1,"value":-25.700014000000003,"_color":"#f50ae9","tween":"quadEaseInOut"},
// 				{"time":2.8,"value":0,"_color":"#2e3712"}
// 			]
// 		}]
// });

function loadTimeline(frames) {
	let layers = [];
	let servoValue = Array(servoSettings.length).fill(0);
	for (let si = 0; si < servoSettings.length; si++) {
		let servo = servoSettings[si];
		if (!servo.srvena)
			continue;
		let values = [];
		let frtime = 0;
		let servoIdx = (si+1).toString();
		for (let fi = 0; fi < frames.length; fi++) {
			let frame = frames[fi];
			if (frame.hasOwnProperty(servoIdx)) {
				let frsrv = frame[servoIdx];
				if (frsrv.srvena) {
					values.push({
						time: (frtime/10.0),
						value: servoValue[si]/1000.0,
						_color: '#' + (Math.random() * 0xffffff | 0).toString(16),
						tween: 'linear'
					});
					servoValue[si] = frsrv.srvpos;
					values.push({
						time: ((frtime+frsrv.srvdur*10)/10.0),
						value: servoValue[si]/1000.0,
						_color: '#' + (Math.random() * 0xffffff | 0).toString(16)
					});
				}
			}
			frtime = frame.frdur * 10;
		}
		if (values.length != 0) {
			layers.push({
				name: servo.srvnam,
				values: values
			});
		}
	}
	let json = {
		version: "1.2.0",
		modified: new Date().toString(),
		layers: layers
	};
	console.log('timeline:',JSON.stringify(json));
	timeliner.load(json);
}

// function animate() {
// 	console.log('hello');
// // 	requestAnimationFrame(animate);

// // 	var w2 = window.innerWidth / 2;
// // 	var h2 = window.innerHeight / 2;

// // 	box.style.transform = 'translateX(' +  (target.x * 100 + w2) + 'px) translateY(' + (target.y * 100 + h2) + 'px) rotate(' + target.rotate * 50 + 'deg)';
// }

// animate();

////////////////////////////////////////

onRuntime = function() {
	console.log('Initialized');
	fetch('/api/load')
		.then(response => response.json())
		.then(data => loadAll(data))
		.catch((error) => {
			console.log('Failed to load data from board:', error);
		});
};
