var RTW_SECURITY ={
	0			:"OPEN",
	1			:"WEP_PSK",
	32769		:"WEP_SHARED",
	2097154		:"WPA_TKIP_PSK",
	2097156		:"WPA_AES_PSK",
	4194308		:"WPA2_AES_PSK",
	4194306		:"WPA2_TKIP_PSK",
	4194310		:"WPA2_MIXED_PSK",
	6291456		:"WPA_WPA2_MIXED",
	268435456	:"WPS_OPEN",
	268435460	:"WPS_SECURE"
};

function createInputForAp(ap) {
	if (ap.essid=="" && ap.rssi==0) return;
	
	var rssi=document.createElement("td");
	rssi.innerHTML="RSSI:"+ap.rssi+"dBm";
	
	var input=document.createElement("input");
	input.type="radio";
	input.name="essid";
	input.value=ap.essid;
	if (currAp==ap.essid) input.checked="1";
	input.id="opt-"+ap.essid;
	var label=document.createElement("label");
	label.htmlFor="opt-"+ap.essid;
	label.textContent=ap.essid;

	var newrow=document.all.networks.insertRow();
	var newcell=newrow.insertCell(0);
	newcell.appendChild(input);
	newcell=newrow.insertCell(1);
	newcell.appendChild(label);
	
	newcell=newrow.insertCell(2);
	newcell.innerHTML=ap.rssi;
	newcell=newrow.insertCell(3);
	newcell.innerHTML=ap.channel;
	newcell=newrow.insertCell(4);
	if (ap.enc ==-1) {
		newcell.innerHTML="UNKNOWN";
	}
	else {
		newcell.innerHTML=RTW_SECURITY[ap.enc];
	}
}

function getSelectedEssid() {
	var e=document.forms.wifiform.elements;
	for (var i=0; i<e.length; i++) {
		if (e[i].type=="radio" && e[i].checked) return e[i].value;
	}
	return currAp;
}

function scanAPs() {
	xhr.open("GET", "wifiscan.cgi");
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			var data=JSON.parse(xhr.responseText);
			currAp=getSelectedEssid();
			if (data.result.inProgress=="0" && data.result.APs.length>1) {
				$("#aps").innerHTML="";
				for (var i=0; i<data.result.APs.length; i++) {
					if (data.result.APs[i].essid=="" && data.result.APs[i].rssi==0) continue;
					//$("#aps").appendChild(createInputForAp(data.result.APs[i]));
					createInputForAp(data.result.APs[i]);					
				}				
				//window.setTimeout(scanAPs, 20000);
			} else {
				window.setTimeout(scanAPs, 1000);
			}
		}
	}
	xhr.send();
}

window.onload=function(e) {
	scanAPs();
};

