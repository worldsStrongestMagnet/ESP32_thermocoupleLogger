// DOM selectors 
const logIntervalObj = document.getElementById('interval');
const logDurationObj = document.getElementById('loggingDuration');
const currentTempDispObj = document.getElementById('currentTempDisp');
const formSubmit = document.getElementById('Submit');
const startLog = document.getElementById('loggingStart');
const clearLog = document.getElementById('clearLog');

async function fetchLatestTemp() {
    try {
        // fetch latest temp from sensor
        const response = await fetch('/latest');
        const temp = await response.text();

        // insert current temp into html
        currentTempDispObj.innerText = temp + "°C";

    } catch (error) {
        console.error('error fetching temp: ', error);
    }
}

async function setLogParams() {
    try {
        // prevent form from clearing - annoying behavior
        event.preventDefault();

        // get values from form
        let logInterval = logIntervalObj.value * 1000;
        let logDuration = logDurationObj.value * 1000;

        if (logInterval < 250) {
            logInterval = 250;
        }

        logDuration = limitLogLen(logInterval, logDuration);

        const data = "inverval=" + encodeURIComponent(logInterval) + "&duration=" + encodeURIComponent(logDuration);

        // add code to push params to server
        fetch('/update-log-specs', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: data
        })
        .then(response => response.text())
        .then(text => alert("Server response: " + text))
        .catch(error => console.error('Error: ', error));

    } catch (error) {
        console.error('error updating params: ', error)
    }
}

function limitLogLen(interval, durration) {
    let totalDatapoints = durration / interval;

    if (totalDatapoints > 10000) {
        return interval * 10000;
    } else {
        return durration;
    }
}

async function clearLogCSV() {
    try{
        const clearLogs = 'clearlogs=' + encodeURIComponent(1);

        fetch('/clear-log', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: clearLogs
        })
        .then(response => response.text())
        .then(text => alert('Server response: ' + text))
        .catch(error => console.error('Error: ', error));
    } catch (error) {
        console.error('error clearing logs: ', error)
    }   
}

async function startLogging() {
    try{
        const startLog = "startlog=" + encodeURIComponent(1);

        fetch('/initiate', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: startLog
        })
        .then(response => response.text())
        .then(text => alert('Server response: '+ text))
        .catch(error => console.error('Error: ', error));
    } catch (error) {
        console.error('error initiating log: ', error);
    }
}

// function clearLogs() {

// }

window.onload = fetchLatestTemp;
formSubmit.onclick = setLogParams;
startLog.onclick = startLogging;
clearLog.onclick = clearLogCSV;