var xhr=j();




function doReboot() {
	xhr.open("GET", "reboot");
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			window.setTimeout(function() {
				location.reload(true);
			}, 3000);
		}
	}
	//ToDo: set timer to 
	xhr.send();
}

function setProgress(amt) {
	$("#progressbarinner").style.width=String(amt*200)+"px";
}

/*
function isNumber(n) {
	return !isNaN(parseFloat(n)) && isFinite(n);
} */


function doUpgrade() {
	var f=$("#file").files[0];
	var autoerase=0;
	var start=0;  //851968
	var inp;
	if (typeof f=='undefined') {
		$("#remark").innerHTML="Can't read file!";
		return
	}
	inp=document.getElementsByName('auto_erase');
	for (var i = 0; i < inp.length; i++) {
		if (inp[i].type === 'checkbox' && inp[i].checked) {
			autoerase=1;
		}
	}
	inp=document.getElementsByName('start_address');
	for (var i = 0; i < inp.length; i++) {
		if (inp[i].type === 'text') {
			start=parseInt(inp[i].value);
		}
	}
	
	if (isNaN(start)|| start <0 || start>=2097152) {
		$("#remark").innerHTML="Invalid start address";
		return false;
	}
	
	xhr.open("POST", "upload");
	xhr.setRequestHeader("x-start", start);
	xhr.setRequestHeader("x-erase", autoerase);
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			setProgress(1);
			if (xhr.responseText!="") {
				$("#remark").innerHTML="Error: "+xhr.responseText;
			} else {
				$("#remark").innerHTML="Uploading done. Select another file or reboot the module.";
				/*doReboot();*/
			}
		}
	}
	if (typeof xhr.upload.onprogress != 'undefined') {
		xhr.upload.onprogress=function(e) {
			setProgress(e.loaded / e.total);
		}
	}
	xhr.send(f);
	return false;
}


window.onload=function(e) {
	setProgress(0);
	var txt="Please upload new binary file.";
			$("#remark").innerHTML=txt;
}