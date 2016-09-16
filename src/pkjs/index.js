 // file: /src/pkjs/index.js
/* globals Pebble, XMLHttpRequest, window, localStorage */

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(event) {
  initiateUpdateNearby();
});

// On the phone, begin listening for a message from the smartwatch
Pebble.addEventListener('appmessage', function(event) {
  // Get the message that was passed
  console.log(JSON.stringify(event.data));
  var message = event.data;
  if (message.getNearby) {
    initiateUpdateNearby();
  }
});

// Send an article for the Pebble to display
function sendArticleToPebble(article, isNew) {
  Pebble.sendAppMessage({
    ARTICLE: article.title,
    IS_NEW: isNew,
    DISTANCE: article.distanceAway.toFixed(1)
  },
  function(e) {
    console.log('Article info sent to Pebble successfully!');
  },
  function(e) {
    console.log('Error sending article info to Pebble!');
  });
}


// Get location via standard geolocation API, and initiate
// search for Wikipedia articles upon success.
var myCoordinates;
var locationOptions = {
  'timeout': 15000,
  'maximumAge': 60000
};
function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
}
function locationSuccess(pos) {
  myCoordinates = pos.coords;
  fetchNearbyUnillustrated(myCoordinates.latitude, myCoordinates.longitude);
}
function initiateUpdateNearby() {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
}

// http://tools.wmflabs.org/articles-by-lat-lon-without-images/index.php?wiki=sv&lat=59.06708056&lon=16.36239722&radius=10000
function fetchNearbyUnillustrated(latitude, longitude) {
  var radius = 10000;
  var url = 'http://tools.wmflabs.org/articles-by-lat-lon-without-images/index.php' +
            '?wiki=en' +
            '&lat=' + latitude +
            '&lon=' + longitude +
            '&radius=' + radius +
            '&reencode=true';
  console.log(url);
  var req = new XMLHttpRequest();
  req.open('GET', url, false);
  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log(req.responseText);
        var response = JSON.parse(req.responseText);
        processNearbyArticles(response);
        // notifyIfNew(response[0]);
      } else {
        console.log('Error');
      }
    }
  };
  req.send(null);
}

// Calculate the distanceAway for each nearby article and find the nearest.
// Results from the wmflabs tool are not ordered by proximity.
var nearestArticle;
var allNearbyArticles;
function processNearbyArticles(articles) {
  articles = articles.map( function(article) {
    var articleCoordinates = {
      latitude: article.lat,
      longitude: article.lon
    };
    article.distanceAway = haversine(myCoordinates, articleCoordinates, { unit: 'km' });
    return article;
  });

  nearestArticle = articles[0];
  allNearbyArticles = articles;

  articles.forEach( function(article) {
    if (article.distanceAway <= nearestArticle.distanceAway) {
      nearestArticle = article;
      console.log(nearestArticle.title);
      console.log(nearestArticle.distanceAway);
    }
  });
  console.log(nearestArticle.title);
  updateNearest(nearestArticle);
}

// Add the article to local storage, if it's not there yet.
// Send the article to Pebble, along with its isNew status.
// Create a notification if it's new.
function updateNearest(article) {
  var title = article.title;
  var storedItem = localStorage.getItem(title);
  console.log(storedItem);
  if (!storedItem) {
    sendArticleToPebble(article, true);
    localStorage.setItem(title, 'true');
    Pebble.showSimpleNotificationOnPebble(title, 'NEW!');
  } else {
    console.log('already done!');
    sendArticleToPebble(article, false);
  }
}

// haversine formula for calculating distance between coordinates
// adapted from https://github.com/njj/haversine/blob/master/haversine.js
function toRadians(degrees) {
  return (degrees * Math.PI / 180);
}

function haversine(start, end, options) {
  options = options || {};

  var radii = {
    km:    6371,
    mile:  3960,
    meter: 6371000
  };

  var R = options.unit in radii ? radii[options.unit] : radii.km;

  var dLat = toRadians(end.latitude - start.latitude);
  var dLon = toRadians(end.longitude - start.longitude);
  var lat1 = toRadians(start.latitude);
  var lat2 = toRadians(end.latitude);

  var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
          Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2);
  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

  if (options.threshold) {
    return options.threshold > (R * c);
  }

  return R * c;
}