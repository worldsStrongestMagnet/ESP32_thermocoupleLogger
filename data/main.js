// DOM selectors
const logIntervalObj = document.getElementById('interval');
const logDurationObj = document.getElementById('loggingDuration');
const currentTempDispObj = document.getElementById('currentTempDisp');

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
    console.log('setLogParams triggered')
    try {
        // prevent form from clearing - annoying behavior
        // event.preventDefault();

        // get values from form
        let logInterval = logIntervalObj.value * 1000;
        let logDuration = logDurationObj.value * 1000;

        if (logInterval < .250) {
            logInterval = .250;
        }

        logDuration = limitLogLen(logInterval, logDuration);

        const data = "intverval=" + encodeURIComponent(logInterval) + "&duration=" + encodeURIComponent(logDuration);

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
    console.log('clearLogCSV event listener triggered')
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
    console.log('start logging event listener triggered')
    try{
        const initiateLog = "startlog=" + encodeURIComponent(1);

        fetch('/initiate', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: initiateLog
        })
        .then(response => response.text())
        .then(text => alert('Server response: '+ text))
        .catch(error => console.error('Error: ', error));
    } catch (error) {
        console.error('error initiating log: ', error);
    }
}


window.onload = fetchLatestTemp;

document.getElementById('submit').addEventListener('click', setLogParams);
document.getElementById('loggingStart').addEventListener('click', startLogging);
document.getElementById('clearLog').addEventListener('click', clearLogCSV);
