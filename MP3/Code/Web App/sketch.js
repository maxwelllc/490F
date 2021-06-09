let video;
let poseNet;
let currentPoses;
let serialOptions = { baudRate: 115200 };
let serial;
let pHtmlMsg;

// Consts for how long to display each bone in stick figure diagram
let figureScale = 3.25;
const shin = 50 * figureScale;
const thigh = 75 * figureScale;
const forearm = 50 * figureScale;
const upperarm = 50 * figureScale;
const torso = 125 * figureScale;
const torsoWidth = 30 * figureScale;

const cameraWidth = 1420;
const cameraHeight = 900;
const cameraXOrigin = (1920 - cameraWidth) / 2;
const cameraYOrigin = (1080 - cameraHeight) / 2;
// max points earned per correct joint
const defaultMaxPoints = 50;

const maxPointsPerPose = defaultMaxPoints * 6; // 6 joints

// Whether to draw the raw data from posenet
const drawSkeleton = false;

// true = normal tomatos, false = cheat mode
const tomatos = true;

// Game states
const gameStates = Object.freeze({ "menu": 1, "minigamePose": 2, "endGame": 3, "pause": 4 });
let currState = gameStates.menu;
// Level that the minigame ends at
const victoryLevel = 10;
// The duration in milliseconds for how long various states within the minigame last, may be scaled 
const gameDurations = Object.freeze({ "poseDuration": 30000, "pauseDuration": 5000 });

// If in minigame state, this is the timestamp of when the most recent pose or pause event will end
let stateEnd;
// The current difficulty level
let level = 0;
// The current target pose
let foot, footPosition, leftHipAngle, rightHipAngle, rightKneeAngle, leftShoulderAngle, leftElbowAngle, rightShoulderAngle, rightElbowAngle;
// date object
let date;
// Timestamp that the tomato should spawn, -1 if no tomato
let tomatoTime = -1;
// the direction of the tomato,  0 = no tomato, 1 = top, 2 = right, 3 = bottom, 4 = left
let tomatoDirection = 0;
// the direction of the shield, 0 = no shield, 1 = top, 2 = right, 3 = bottom, 4 = left
let shieldDirection = 0;
// Lives remaining
let livesRemaining = 3;



// images
let stage;
let title;
let salsa;
let shield;
let tomato; // https://www.kindpng.com/imgv/TJoJRbx_transparent-tomatoe-png-tomato-splat-transparent-png-download/
let failureStage;
let successStage;

// sound effects
let success; // https://freesound.org/people/Bertrof/sounds/131660/
let failure; // https://freesound.org/people/SgtPepperArc360/sounds/341732/
let timeRunningOut; // https://freesound.org/people/meatball4u/sounds/17216/
let splat; // https://freesound.org/people/Rock%20Savage/sounds/81042/
let countdown; // https://freesound.org/people/zorrodg/sounds/103227/
let monkeyDance; // https://www.youtube.com/watch?v=ahklLaDDCCQ

// the most recent score, in case theres brief loss of pose during judging 
let lastScore = 0;

// total score 
let score = 0;

// Whether the button is pressed
let buttonPressed = false;

// Constraints for where the pose can originate
const minFootPos = 215;
const maxFootPos = 420;

// Where the poses will originate on the y level if theres no person on frame
let footYLevel = 400;

function preload() {
  stage = loadImage('stage.png');
  salsa = loadImage('salsa.gif');
  title = loadImage('title.png');
  shield = loadImage('shield.png');
  tomato = loadImage('tomato.png');
  failureStage = loadImage('booStage.png');
  successStage = loadImage('successState.png');

  correct = loadSound('correct.wav');
  failure = loadSound('failure.wav');
  splat = loadSound('splat.wav');
  beeping = loadSound('beeping.wav');
  countdown = loadSound('countdown.mp3');
  monkeyDance = loadSound('funkyMonkey.mp3')
}

function setup() {

  console.log("Starting!");
  // Setup Web Serial using serial.js
  serial = new Serial();
  serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
  serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
  serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
  serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

  // If we have previously approved ports, attempt to connect with them
  serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);


  // Add in a lil <p> element to provide messages. This is optional
  pHtmlMsg = createP("Click anywhere on this page to open the serial connection dialog");
  pHtmlMsg.hide();
  video = createCapture(VIDEO);
  console.log(video.size());

  createCanvas(1920, 1080);

  video.hide();
  options = {
    architecture: 'ResNet50',
    imageScaleFactor: 0.5,
    outputStride: 32,
    flipHorizontal: false,
    minConfidence: 0.5,
    maxPoseDetections: 1,
    scoreThreshold: 0.5,
    nmsRadius: 20,
    detectionType: 'single',
    inputResolution: 513,
    multiplier: 1.0,
    quantBytes: 4
  };
  poseNet = ml5.poseNet(video, options, onPoseNetModelReady);
  poseNet.on('pose', onPoseDetected); // call onPoseDetected when pose detected

  genPose(0);
  angleMode(RADIANS);
  stateEnd = Date.now() + gameDurations.poseDuration;
  level = 1;
}

function draw() {
  console.log(currState);
  switch (currState) {
    case gameStates.menu:
      menu();
      break;
    case gameStates.minigamePose:
      minigame();
      break;
    case gameStates.endGame:
      endGame();
      break;
    case gameStates.pause:
      pause();
      break;


  }
}
function menu() {
  bgColor = color(255, 11, 30);
  background(bgColor);
  tint(255, 127);
  image(stage, 0, 0, width, height);
  tint(255, 255);
  textAlign(CENTER);
  textStroke = color(0, 0, 0);
  stroke(textStroke);
  strokeWeight(2);

  imageMode(CENTER);
  image(title, 1920 / 2, 200);
  image(salsa, 1920 / 2, 1080 - 300, 500, 500);
  imageMode(CORNER);
  textSize(32);
  text('Designate Player 1 to be the Monkey and Player 2 to be the Director.\nThe Monkey will hold the controller and stand in front of the camera.\nThe Monkey will make the poses as instructed, and alert the Director if their controller rumbles to indicate a tomato coming! \nThe Director will sit at the computer and guide the Monkey to perform a pose and use the arrow keys to protect from tomatos.\n\nPress the button on the controller to begin!', 1920 / 2, 1080 / 3);

  if (!monkeyDance.isLooping()) {
    monkeyDance.setLoop(true);
    monkeyDance.setVolume(0.45);
    monkeyDance.play();
  }
  if (buttonPressed == true) {
    monkeyDance.stop();
    buttonPressed = false;
    currState = gameStates.minigamePose;
    stateEnd = Date.now() + gameDurations.poseDuration + 500;
    level = 0;
    genPose(level);
    tomatoTime = Date.now() + gameDurations.poseDuration / 2;
    tomatoDirection = 3;
    livesRemaining = 3;
  }

  serialWrite(currState, 0, livesRemaining);
}

function endGame() {
  textSize(70);
  textStroke = color(0, 0, 0);
  stroke(textStroke);
  strokeWeight(2);
  if (livesRemaining <= 0) { // Lost!
    bgColor = color(255, 0, 0);
    background(bgColor);
    text("YOU LOST AT LEVEL " + level + " OUT OF 10", 1920 / 2, 200);

  } else { // Won!

    bgColor = color(0, 255, 0);
    background(bgColor);
    text("YOU DANCED THROUGH ALL THE LEVELS!", 1920 / 2, 200);
  }


  textSize(40);
  text("You earned " + Math.floor(score) + " pose-points out of a possible " + (victoryLevel * maxPointsPerPose), 1920 / 2, 600)

  text("Press the controller button to try again!", 1920 / 2, 700);
}
// The actual pose minigame
function minigame() {
  // Play the beeping sound effect right before we end
  whenToPlayBeeping = stateEnd - (beeping.duration() * 1000);
  if (whenToPlayBeeping < Date.now() && Date.now() < whenToPlayBeeping + 500) {
    playSound(beeping);
  }

  if (stateEnd < Date.now() || buttonPressed == true) {
    // End round!
    if (tomatos == false) {
      shieldDirection = tomatoDirection;
      
    }
    
    image(video, cameraXOrigin, cameraYOrigin, cameraWidth, cameraHeight, cameraXOrigin, cameraYOrigin, cameraWidth, cameraHeight);
    score = judgeCurrentPose();
    safe = judgeTomatoBlocking(); 
    
    monkeyDance.stop();
    if (score > maxPointsPerPose / 2 && safe) { // Proceed normally
      correctPose();
    } else { // lose a life
      incorrectPose(safe);
    }

    currState = gameStates.pause;
    stateEnd = Date.now() + gameDurations.pauseDuration - Math.max((level * 200), gameDurations.pauseDuration / 4);
    return;
  } else { // round is in progress

    if (!monkeyDance.isPlaying()) {
      monkeyDance.play();
      monkeyDance.jump(11);
    }
    image(stage, 0, 0, width, height);
    tint(255, 200)
    image(video, cameraXOrigin, cameraYOrigin, cameraWidth, cameraHeight, cameraXOrigin, cameraYOrigin, cameraWidth, cameraHeight);
    tint(255, 255);
    // Gradient-ize the stick figure based on how close you are 
    c1 = color(255, 0, 0, 128);
    c2 = color(0, 255, 0, 128);
    let figureColor = lerpColor(c1, c2, judgeCurrentPose() / (defaultMaxPoints * 6));
    stroke(figureColor);
    drawTargetPose();
    drawShield();

    strokeWeight(10);
    fill(255, 0, 0);
    heartSize = 200;
    for (i = 0; i < livesRemaining; i++) {
      heart((i * (heartSize - 50)) + heartSize / 2, heartSize / 2 - 50, heartSize / 2);
    }
    if (tomatoTime != -1 && Date.now() > tomatoTime && Date.now() < tomatoTime + 200) { // It's tomato time
      // send tomato alert 
      serialWrite(currState, tomatoDirection, livesRemaining);
      console.log("Tomato time from " + tomatoDirection);
    }


    // Pose renderer "borrowed" from example 
    // https://makeabilitylab.github.io/physcomp/communication/ml5js-serial.html#recognizing-human-poses-with-posenet
    if (drawSkeleton && currentPoses && currentPoses[0]) {

      pose = currentPoses[0];

      poseIndex = 0;
      // Draw skeleton
      strokeWeight(2);
      const skeleton = pose.skeleton;
      for (let j = 0; j < skeleton.length; j += 1) {
        const partA = skeleton[j][0];
        const partB = skeleton[j][1];
        line(partA.position.x, partA.position.y, partB.position.x, partB.position.y);
      }

      // Draw keypoints with text
      const kpFillColor = color(255, 255, 255, 200);
      const textColor = color(255, 255, 255, 230);
      const kpOutlineColor = color(0, 0, 0, 150);
      strokeWeight(1);

      const keypoints = pose.pose.keypoints;
      const kpSize = 10;
      const kpTextMargin = 2;
      let xPoseLeftMost = width;
      let xPoseRightMost = -1;
      let yPoseTop = height;
      let yPoseBottom = -1;
      for (let j = 0; j < keypoints.length; j += 1) {
        // A keypoint is an object describing a body part (like rightArm or leftShoulder)
        const kp = keypoints[j];

        // check for maximum extents
        if (xPoseLeftMost > kp.position.x) {
          xPoseLeftMost = kp.position.x;
        } else if (xPoseRightMost < kp.position.x) {
          xPoseRightMost = kp.position.x;
        }

        if (yPoseBottom < kp.position.y) {
          yPoseBottom = kp.position.y;
        } else if (yPoseTop > kp.position.y) {
          yPoseTop = kp.position.y;
        }

        fill(kpFillColor);
        noStroke();
        circle(kp.position.x, kp.position.y, kpSize);

        fill(textColor);
        textAlign(LEFT);
        let xText = kp.position.x + kpSize + kpTextMargin;
        let yText = kp.position.y;
        if (kp.part.startsWith("right")) {
          textAlign(RIGHT);
          xText = kp.position.x - (kpSize + kpTextMargin);
        }
        textStyle(BOLD);
        text(kp.part, xText, yText);
        textStyle(NORMAL);
        yText += textSize();
        text(int(kp.position.x) + ", " + int(kp.position.y), xText, yText);

        yText += textSize();
        text(nf(kp.score, 1, 2), xText, yText);
        //console.log(keypoint);
        // Only draw an ellipse is the pose probability is bigger than 0.2
        //if (keypoint.score > 0.2) {

        noFill();
        stroke(kpOutlineColor);
        circle(kp.position.x, kp.position.y, kpSize);
      }

      const boundingBoxExpandFraction = 0.1;
      let boundingBoxWidth = xPoseRightMost - xPoseLeftMost;
      let boundingBoxHeight = yPoseBottom - yPoseTop;
      let boundingBoxXMargin = boundingBoxWidth * boundingBoxExpandFraction;
      let boundingBoxYMargin = boundingBoxHeight * boundingBoxExpandFraction;
      xPoseRightMost += boundingBoxXMargin / 2;
      xPoseLeftMost -= boundingBoxXMargin / 2;
      yPoseTop -= boundingBoxYMargin / 2;
      yPoseBottom += boundingBoxYMargin / 2;

      noStroke();
      fill(textColor);
      textStyle(BOLD);
      textAlign(LEFT, BOTTOM);
      const strPoseNum = "Pose: " + (poseIndex + 1);
      text(strPoseNum, xPoseLeftMost, yPoseTop - textSize() - 1);
      const strWidth = textWidth(strPoseNum);
      textStyle(NORMAL);
      text("Confidence: " + nf(pose.pose.score, 0, 1), xPoseLeftMost, yPoseTop);

      noFill();
      stroke(kpFillColor);
      rect(xPoseLeftMost, yPoseTop, boundingBoxWidth + boundingBoxXMargin,
        boundingBoxHeight + boundingBoxYMargin);
    }

  }


}

function heart(x, y, size) {
  beginShape();
  vertex(x, y);
  bezierVertex(x - size / 2, y - size / 2, x - size, y + size / 3, x, y + size);
  bezierVertex(x + size, y + size / 3, x + size / 2, y - size / 2, x, y);
  endShape(CLOSE);
}

function correctPose() {

  image(successStage, 0, 0, width, height);
  playSound(correct);

  serialWrite(currState, 0, livesRemaining);
}

function incorrectPose(safe) {

  if (safe) {
    image(failureStage, 0, 0, width, height);
    playSound(failure);
  }
  else {
    playSound(splat);
  }
  livesRemaining--;

  serialWrite(currState, 0, livesRemaining);
}

function pause() {
  if (buttonPressed == true || Date.now() > stateEnd) {
    currState = gameStates.minigamePose;
    buttonPressed = false;
    level++;

    // Give 1s less time to pose based on difficulty, minimum of base/8
    stateEnd = Date.now() + gameDurations.poseDuration - Math.max((level * 1000), gameDurations.poseDuration / 8);

    // Calculate a number 0-100
    tomatoRNG = Math.floor(Math.random() * 100);

    // If tomato RNG is below this number, a tomato will spawn this pose!
    threshold = Math.floor(Math.random() * (10 + level * 10));
    if (tomatoRNG < threshold) {
      console.log("Tomato time decided");
      // tomatoTime will occur closer to the start with lower difficulty
      tomatoTime = Date.now() + Math.floor(Math.random() * level * 500);
      // but if we try to set before now+1000 or after state end-1000, we can constrain
      tomatoTime = Math.min(Math.max(tomatoTime, Date.now() + 1000), stateEnd - 1000);

      tomatoDirection = 1 + Math.floor(Math.random() * 5);
    } else {
      tomatoTime = -1;
      tomatoDirection = 0;
    }
    if (level > victoryLevel || livesRemaining <= 0) {
      currState = gameStates.endGame;
    }
    genPose(level);
    serialWrite(currState, 0, livesRemaining);

  } else {
    textSize(100);
    bgColor = color(255, 255, 255);
    fill(bgColor);
    strokeWeight(2);
    rect(0, 0, 1920, 150)

    textStroke = color(0, 0, 0);
    stroke(textStroke);
    fill(textStroke);
    text("Next pose in " + Math.floor((stateEnd - Date.now()) / 1000) + "...", 1920 / 2, 100);
  }
}

/** Generate a random pose, where difficulty is an int ideally from 0-20 that will scale the angle of each random pose
 * 
 * @param {int} difficulty 
 */
function genPose(difficulty) {

  if (Math.random() < 0.5) {
    foot = 0; // Left foot on the ground
    leftHipAngle = toRadians(Math.floor(Math.random() * (30 + difficulty)));
    leftKneeAngle = toRadians(90 - Math.floor(Math.random() * (10 + difficulty)));
    rightHipAngle = toRadians(90 - Math.floor(Math.random() * (50 + difficulty)));
    rightKneeAngle = toRadians(90 - Math.floor(Math.random() * (50 + difficulty)));
  } else {
    foot = 1; // Right foot

    leftHipAngle = toRadians(90 - Math.floor(Math.random() * (50 + difficulty)));
    leftKneeAngle = toRadians(90 - Math.floor(Math.random() * (50 + difficulty)));
    rightHipAngle = toRadians(Math.floor(Math.random() * (30 + difficulty)));
    rightKneeAngle = toRadians(90 - Math.floor(Math.random() * (10 + difficulty)));
  }

  // Generate angles for each arm

  leftShoulderAngle = (Math.random() < 0.5 ? -1 : 1) * toRadians(Math.floor(Math.random() * (50 + difficulty)));
  leftElbowAngle = leftShoulderAngle - toRadians((Math.random() < 0.5 ? -1 : 1) * (Math.floor(Math.random() * (90 + difficulty))));

  rightShoulderAngle = toRadians(180) + (Math.random() < 0.5 ? -1 : 1) * toRadians(Math.floor(Math.random() * (50 + difficulty)));
  rightElbowAngle = rightShoulderAngle - toRadians((Math.random() < 0.5 ? -1 : 1) * (Math.floor(Math.random() * (90 + difficulty))));

  // Generate a position for the foot - center if no pose yet, +- difficulty if pose
  footPosition = Math.floor(Math.random() * (level * 10));
  if (Math.random() < 0.5) {
    footPosition = footPosition * -1;
  }
  if (currentPoses && currentPoses[0]) {
    footPosition = footPosition + currentPoses[0].pose.leftAnkle;
  } else {
    footPosition = (width / 2) + footPosition;
  }
  footPosition = Math.min(Math.max(footPosition, minFootPos), maxFootPos);
  /*
  console.log("0 = left foot dominant, 1 = right foot dominant: " + foot);
  console.log("left hip:" + toDegrees(leftHipAngle));
  console.log("left knee: " + toDegrees(leftKneeAngle));
  console.log("right hip:" + toDegrees(rightHipAngle));
  console.log("right knee:" + toDegrees(rightKneeAngle));
  console.log("left shoulder: " + toDegrees(leftShoulderAngle));
  console.log("left elbow angle:" + toDegrees(leftElbowAngle));
  console.log("right shoulder: " + toDegrees(rightShoulderAngle));
  console.log("right elbow angle: " + toDegrees(rightElbowAngle)); */
}

function playSound(sound) {
  if (sound.isLoaded() && !sound.isPlaying()) {
    console.log("Playing sound " + sound);
    sound.play();
    sound.setLoop(false);
  }
}
function drawShield(drawTomato = false) {
  let x = 1920 / 2
  let y = 1080 / 2;
  let size = 300;

  imageMode(CENTER);
  if (shieldDirection == 1) {
    x = 1920 / 2;
    y = size / 2;
  } else if (shieldDirection == 2) {
    x = 1920 - size / 2;
    y = 1080 / 2;
  } else if (shieldDirection == 3) {
    x = 1920 / 2;
    y = 1080 - size / 2;
  }
  else if (shieldDirection == 4) {
    x = size / 2;
    y = 1080 / 2;
  }
  if (shieldDirection != 0) {

    image(shield, x, y, size, size);

  }
  if (drawTomato) {
    image(tomato, x, y, size / 2, size / 2)
  }
  imageMode(CORNER);
}
/**
 * Draws the current target pose as a stick figure
 */
function drawTargetPose() {

  strokeWeight(20 * figureScale);
  const kpSize = 10;

  if (currentPoses && currentPoses[0]) {
    if (foot == 0) { // get left foot pos

      lastX = currentPoses[0].pose.leftAnkle.x;
      lastY = currentPoses[0].pose.leftAnkle.y;
    } else { // Right foot pos

      lastX = currentPoses[0].pose.rightAnkle.x;
      lastY = currentPoses[0].pose.rightAnkle.y;
    }
  } else {

    lastX = footPosition;
    lastY = footYLevel;
  }
  lastX = 1920 / 2;
  lastY = 1080 - 100

  if (foot == 0) { // Left foot on the ground
    // left shin
    line(lastX, lastY, lastX, lastY - shin);
    lastX;
    lastY -= shin;
    // left thigh
    line(lastX, lastY, lastX - thigh * cos(leftKneeAngle), lastY - thigh * sin(leftKneeAngle));
    lastX -= thigh * cos(leftKneeAngle);
    lastY -= thigh * sin(leftKneeAngle);
    // lowerbound torso
    line(lastX, lastY, lastX - torsoWidth * cos(leftHipAngle), lastY - torsoWidth * sin(leftHipAngle));
    torsoLowerMidX = lastX - torsoWidth * cos(leftHipAngle) / 2;
    torsoLowerMidY = lastY - torsoWidth * sin(leftHipAngle) / 2;


    lastX -= torsoWidth * cos(leftHipAngle);
    lastY -= torsoWidth * sin(leftHipAngle);

    //active thigh
    line(lastX, lastY, lastX - thigh * cos(rightHipAngle), lastY + thigh * sin(rightHipAngle));
    lastX -= thigh * cos(rightHipAngle);
    lastY += thigh * sin(rightHipAngle);

    //active shin
    line(lastX, lastY, lastX - shin * cos(rightKneeAngle), lastY + shin * sin(rightKneeAngle));
    lastX -= shin * cos(rightKneeAngle);
    lastY += shin * sin(rightKneeAngle);

    // Angle to draw torso line
    perpendicularAngle = leftHipAngle + toRadians(90);
  } else { // right foot on the ground
    // right shin
    line(lastX, lastY, lastX, lastY - shin);
    lastX;
    lastY -= shin;
    // right thigh
    line(lastX, lastY, lastX + thigh * cos(rightKneeAngle), lastY - thigh * sin(rightKneeAngle));
    lastX += thigh * cos(rightKneeAngle);
    lastY -= thigh * sin(rightKneeAngle);
    // lowerbound torso
    line(lastX, lastY, lastX - torsoWidth * (cos(toRadians(180) - rightHipAngle)), lastY - torsoWidth * sin(toRadians(180) - rightHipAngle));
    torsoLowerMidX = lastX - torsoWidth * (cos(toRadians(180) - rightHipAngle) / 2);
    torsoLowerMidY = lastY - torsoWidth * sin(toRadians(180) - rightHipAngle) / 2;


    lastX -= torsoWidth * (cos(toRadians(180) - rightHipAngle));
    lastY -= torsoWidth * sin(toRadians(180) - rightHipAngle);

    //left thigh
    line(lastX, lastY, lastX + thigh * cos(leftHipAngle), lastY + thigh * sin(leftHipAngle));
    lastX += thigh * cos(leftHipAngle);
    lastY += thigh * sin(leftHipAngle);

    //left shin
    line(lastX, lastY, lastX - shin * cos(leftKneeAngle), lastY + shin * sin(leftKneeAngle));
    lastX -= shin * cos(leftKneeAngle);
    lastY += shin * sin(leftKneeAngle);

    // Angle to draw torso line
    perpendicularAngle = toRadians(90) - rightHipAngle;
  }

  // Torso line
  line(torsoLowerMidX, torsoLowerMidY, torsoLowerMidX - torso * cos(perpendicularAngle), torsoLowerMidY - torso * sin(perpendicularAngle));
  torsoEndX = torsoLowerMidX - torso * cos(perpendicularAngle);
  torsoEndY = torsoLowerMidY - torso * sin(perpendicularAngle);
  armOriginX = torsoLowerMidX - (torso - 40 * figureScale) * cos(perpendicularAngle);
  armOriginY = torsoLowerMidY - (torso - 40 * figureScale) * sin(perpendicularAngle);
  // head
  circle(torsoEndX, torsoEndY, 30 * figureScale);

  // left upperarm
  line(armOriginX, armOriginY,
    armOriginX + upperarm * cos(leftShoulderAngle), armOriginY + upperarm * sin(leftShoulderAngle));

  lastX = armOriginX + upperarm * cos(leftShoulderAngle);
  lastY = armOriginY + upperarm * sin(leftShoulderAngle)
  //left forearm
  line(lastX, lastY, lastX + forearm * cos(leftElbowAngle), lastY + forearm * sin(leftElbowAngle));

  // right upperarm
  line(armOriginX, armOriginY,
    armOriginX + upperarm * cos(rightShoulderAngle), armOriginY + upperarm * sin(rightShoulderAngle));

  lastX = armOriginX + upperarm * cos(rightShoulderAngle);
  lastY = armOriginY + upperarm * sin(rightShoulderAngle)
  // right forearm
  line(lastX, lastY, lastX + forearm * cos(rightElbowAngle), lastY + forearm * sin(rightElbowAngle));


}

function judgeTomatoBlocking() {
  if (tomatoDirection != 0) {
    if (tomatoDirection == shieldDirection) {
      drawShield(true);
      return true;
    } else {

      drawShield();
      if (currentPoses && currentPoses[0]) {
        imageMode(CENTER);
        image(tomato, currentPoses[0].pose.nose.x, currentPoses[0].pose.nose.y, 200, 200);
        imageMode(CORNER);
      }
      else {
        imageMode(CENTER);
        image(tomato, 1920 / 2, 1080 / 2, 200, 200);
        imageMode(CORNER);
      }
      return false;
    }
  }
  return true;

}
function judgeCurrentPose() {
  let totalScore = 0;
  if (currentPoses && currentPoses[0]) {
    pose = currentPoses[0].pose;

    // left leg
    currLeftKnee = pointsToAngle(pose.leftKnee, pose.leftAnkle);
    //currLeftHip = pointsToAngle(pose.leftKnee, pose.leftHip);

    // left  arm 
    currLeftUpperArm = pointsToAngle(pose.leftShoulder, pose.leftElbow);
    currLeftForeArm = pointsToAngle(pose.leftElbow, pose.leftWrist);

    // right leg
    currRightKnee = pointsToAngle(pose.rightKnee, pose.rightAnkle);
    //currRightHip = pointsToAngle(pose.rightKnee, pose.rightHip);

    // right arm
    currRightUpperArm = pointsToAngle(pose.rightShoulder, pose.rightElbow);
    currRightForeArm = pointsToAngle(pose.rightElbow, pose.rightWrist);

    // Torso lower bound line
    currTorsoLine = pointsToAngle(pose.leftHip, pose.rightHip);

    //console.log("part: actual angle, target angle");
    //console.log("leftKnee " + toDegrees(currLeftKnee) + ", " + toDegrees(leftKneeAngle));
    totalScore += compareAngles(currLeftKnee, leftKneeAngle, defaultMaxPoints);


    //console.log("leftUpper" + toDegrees(currLeftUpperArm) + ", " + toDegrees(leftShoulderAngle));
    totalScore += compareAngles(currLeftUpperArm, leftShoulderAngle, defaultMaxPoints);

    //console.log("leftFore" + toDegrees(currLeftForeArm) + ", " + toDegrees(leftElbowAngle));
    totalScore += compareAngles(currLeftForeArm, leftElbowAngle, defaultMaxPoints);

    //console.log("rightKnee " + toDegrees(currRightKnee) + ", " + toDegrees(rightKneeAngle));
    totalScore += compareAngles(currRightKnee, rightKneeAngle, defaultMaxPoints);

    //console.log("rightUpper" + toDegrees(currRightUpperArm) + ", " + toDegrees(rightShoulderAngle));
    totalScore += compareAngles(currRightUpperArm, rightShoulderAngle, defaultMaxPoints);

    //console.log("rightFore" + toDegrees(currRightForeArm) + ", " + toDegrees(rightElbowAngle));
    totalScore += compareAngles(currRightForeArm, rightElbowAngle, defaultMaxPoints);





    lastScore = totalScore;
    return totalScore;
  } else {
    return lastScore;
  }
}


function compareAngles(A, B, maximumPoints) {

  // Convert to all positive angles 
  A = A < 0 ? A + toRadians(360) : A;
  B = B < 0 ? B + toRadians(360) : B;

  // convert back to 0-180, -180-0
  A = A > 180 ? A - toRadians(360) : A;
  B = B > 180 ? B - toRadians(360) : B;

  vectorA = p5.Vector.fromAngle(A);
  vectorB = p5.Vector.fromAngle(B);

  // The difference in degrees, 0 to 180
  diff = toDegrees(Math.abs(vectorA.angleBetween(vectorB)));

  points = map(diff, 45, 0, 0, maximumPoints, true);
  //console.log("Diff: " + diff + ", pts: " + points);
  return points;
}


/**
 * Finds the angle ABC in radians
 * Stolen from https://stackoverflow.com/questions/17763392/how-to-calculate-in-javascript-angle-between-3-points  
 * 
 * @param {*} Ax 
 * @param {*} Ay 
 * @param {*} Bx 
 * @param {*} By 
 * @param {*} Cx 
 * @param {*} Cu 
 * @returns 
 */
function pointsToAngle(A, B) {


  vector = createVector(B.x - A.x, B.y - A.y);
  if (toDegrees(vector.heading()) < 0) {
    return vector.heading() + toRadians(360);
  }
  else {
    return vector.heading();
  }
  /*Cx = Bx+20;
  Cy = By;
  var AB = Math.sqrt(Math.pow(Bx - Ax, 2) + Math.pow(By - Ay, 2));
  var BC = Math.sqrt(Math.pow(Bx - Cx, 2) + Math.pow(By - Cy, 2));
  var AC = Math.sqrt(Math.pow(Cx - Ax, 2) + Math.pow(Cy - Ay, 2));
  return Math.acos((BC * BC + AB * AB - AC * AC) / (2 * BC * AB));*/
}

function toDegrees(rad) {
  return rad * (180 / Math.PI);
}

function toRadians(deg) {
  return deg * (Math.PI / 180);
}




function onPoseNetModelReady() {
  print("The PoseNet model ready");
}

function onPoseDetected(poses) {
  //print("On new poses detected!");
  if (poses) {
    //print("We found " + poses.length + " humans");
  }

  currentPoses = poses;
}



/**
 * Callback function by serial.js when there is an error on web serial
 * 
 * @param {} eventSender 
 */
function onSerialErrorOccurred(eventSender, error) {
  console.log("onSerialErrorOccurred", error);
  pHtmlMsg.html(error);
}

/**
 * Callback function by serial.js when web serial connection is opened
 * 
 * @param {} eventSender 
 */
function onSerialConnectionOpened(eventSender) {
  console.log("onSerialConnectionOpened");
  pHtmlMsg.html("Serial connection opened successfully");
}

/**
 * Callback function by serial.js when web serial connection is closed
 * 
 * @param {} eventSender 
 */
function onSerialConnectionClosed(eventSender) {
  console.log("onSerialConnectionClosed");
  pHtmlMsg.html("onSerialConnectionClosed");
}

/**
 * Callback function serial.js when new web serial data is received
 * Serial data should be ASCII in the form of a comma separated list of values in the following order
 * buttonPressed(int, 0/1)
 * 
 * @param {*} eventSender 
 * @param {String} newData new data received over serial
 */
function onSerialDataReceived(eventSender, newData) {
  console.log("onSerialDataReceived", newData);
  pHtmlMsg.html("onSerialDataReceived: " + newData);

  if (newData == "0") {
    buttonPressed = false;
  } else if (newData == "1") {
    buttonPressed = true;
  }

}

/**
 * Called automatically by the browser through p5.js when mouse clicked
 */
function mouseClicked() {
  if (!serial.isOpen()) {
    serial.connectAndOpen(null, serialOptions);
  }
}

function keyPressed() {
  console.log("Key " + keyCode + " pressed");
  if (keyCode == UP_ARROW) {
    shieldDirection = 1;
  } else if (keyCode == RIGHT_ARROW) {
    shieldDirection = 2;
  }
  else if (keyCode == DOWN_ARROW) {
    shieldDirection = 3;
  } else if (keyCode == LEFT_ARROW) {
    shieldDirection = 4;
  }

}
function keyReleased() {
  shieldDirection = 0;
}

/**
 * 
 * @param {int} currState The game state, use enum
 * @param {int} tomatoDirection 0 = no tomato, 1 = top, 2 = right, 3 = bottom, 4 = left
 * @param {int} livesRemaining lives remaining
 * @returns 
 */
async function serialWrite(currState, tomatoDirection, livesRemaining) {


  if (serial.isOpen()) {
    let strData = currState + "," + tomatoDirection + "," + livesRemaining;

    serial.writeLine(strData);
    console.log("Writing: " + strData);
  }



  return
}