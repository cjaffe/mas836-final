var width = 600,
    height = 700;

var width2 = 25,
    height2 = 700;

var width3 = 25,
    height3 = 700;

var map_svg = d3.select( "#map" )
  .append( "svg" )
  .attr( "width", width )
  .attr( "height", height );

var readings_svg = d3.select("#readings")
  .append("svg")
  .attr("width", width2)
  .attr("height", height2);

var circ_svg = d3.select("#qual-circles")
  .append("svg")
  .attr("width", width3)
  .attr("height", height3);

var g = map_svg.append( "g" );
var g_readings = readings_svg.append("g");
var g_qual_cir = circ_svg.append("g");

drawCircles();
setInterval(drawCircles, 750);

function drawCircles() {
  g_readings.selectAll("circle")
  .remove();
  g_readings.selectAll("cirle")
  .data(data)
  .enter()
  .append("circle")
  .attr("r", "10px")
  .attr("cx", "10px")
  .attr("cy", function(d, i) {
    return 77+ i*38;
  })
  .attr("fill", function(d, i) {
    //console.log(d);
    if (i === 0 || i === 4) { return "white"; }
    else if (d < -400) { return colors[0]; }
    else if (d < -300) { return colors[1]; }
    else if (d < -200) { return colors[2]; }
    else if (d < -100) { return colors[3]; }
    else if (d < 0) { return colors[4]; }
    else if (d < 100) { return colors[5]; }
    else if (d < 200) { return colors[6]; }
    else if (d < 300) { return colors[7]; }
    else { return colors[8]; }
  });
}

updateQuality();
updateQualCircle();
setInterval(updateQuality, 500);
setInterval(detectPothole, 250);

function updateQuality() {
  quality = qual_data.reduce(function(result, data, i) {
    return result + Math.abs(data - qual_data_prev[i]);
  }, 0);
  quality = 1000 - Math.floor(quality);
  qual_arr[1] = quality;
  document.getElementById("overall-quality").innerHTML = quality;
  qual_data_prev = qual_data.slice();
  updateQualCircle();
}

function detectPothole() {
  if ((piezo1 - piezo1_prev) + (piezo2 - piezo2_prev) > 300) {
    pothole_count += 1;
    document.getElementById("pothole-count").innerHTML = pothole_count;
    qual_arr[3] = pothole_count;
  }
  piezo1_prev = piezo1;
  piezo2_prev = piezo2;
}

function updateQualCircle() {
  g_qual_cir.selectAll("circle")
  .remove();
  g_qual_cir.selectAll("cirle")
    .data(qual_arr)
    .enter()
    .append("circle")
    .attr("r", "10px")
    .attr("cx", "10px")
    .attr("cy", function(d, i) {
      return 77+ i*38;
    })
    .attr("fill", function(v, i) {
      var d;
      if (i === 1) { d = v; }
      else if (i === 3) { d = 1000 - v*50; }
      if (i === 0 || i === 2) { return "white"; }
      else if (d > 900) { return colors[0]; }
      else if (d > 800) { return colors[1]; }
      else if (d > 700) { return colors[2]; }
      else if (d > 600) { return colors[3]; }
      else if (d > 500) { return colors[4]; }
      else if (d > 400) { return colors[5]; }
      else if (d > 300) { return colors[6]; }
      else if (d > 200) { return colors[7]; }
      else { return colors[8]; }
    });
}

//
// MAP FUNCTIONS
// 
var points = []; 

var sortedPoints, shortSortedPoints;

function toggleMap() {
  $("#map").toggle();
  g.selectAll("circle").remove();
  mapDefaultAction();
}

var albersProjection = d3.geo.albers()
  .scale( 450000 )
  .rotate( [71.057,0] )
  .center( [-0.055, 42.365477] )
  .translate( [width/2,height/2] );

var geoPath = d3.geo.path()
    .projection( albersProjection );

g.selectAll( "path" )
  .data( neighborhoods_json.features )
  .enter()
  .append( "path" )
  .attr( "fill", colors[3] )
  .attr( "d", geoPath );

var streets = streets_json.features;
var sortedStreets = streets.sort( function(a,b) 
    { return b.id > a.id; }
  );

// give fake data values to each point
sortedStreets.map(function(d) {
  d.geometry.coordinates.map(function(d) {
    d.value = Math.floor((10*Math.random()* 100) / 100);
  });
});

//draw streets
g.selectAll( "path" )
  .data( sortedStreets )
  .enter()
  .append( "path" )
  .attr( "stroke", colors[1])
  .attr("stroke-width", 1)
  .attr("fill", "none")
  .attr( "d", geoPath )
  .on("click", function(d) {
    showStreetData(d);
  });

  function getThisStreet(id) {
  return streets.filter(function(d) {
    return d.properties.Street_ID === id;
  });
}

// get points for street with certain ID
function getThisStreetShortSortedPoints(streetSegments) {
  //console.log(streetSegments);
  streetSegments.map(function(d) {
      points = points.concat(d.geometry.coordinates);

      sortedPoints = points.sort( function(a,b) { return a[1] - b[1]; });
      shortSortedPoints = sortedPoints.filter(function(d, i) { return i%4 === 0; });
    });
}

var colors2 = ['#bdd7e7','#6baed6','#3182bd','#08519c'];

function updateMap() {
  g.selectAll("circle")
  .data(shortSortedPoints)
  .enter()
  .append("circle")
  .transition()
  .delay(function (d, i) { return (i+1)*60;})
  .attr("cx", function (d) { //console.log(d); 
    return albersProjection(d)[0]; })
  .attr("cy", function (d) { return albersProjection(d)[1]; })
  .attr("r", "2px")
  .attr("fill", function(d) { 
    if (d.value < 2) { return colors2[0]; }
    else if (d.value < 5) { return colors2[1]; }
    else if (d.value < 7) { return colors2[2]; }
    else { return colors2[3]; } });
}

function mapDefaultAction() {
  getThisStreetShortSortedPoints(getThisStreet(520.0));
  updateMap();
}

