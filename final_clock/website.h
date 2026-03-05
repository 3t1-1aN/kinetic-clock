const char INDEX_HTML[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background: #f0f2f5; padding: 20px; }
    .card { background: white; padding: 20px; border-radius: 15px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); max-width: 400px; margin: auto; }
    .btn { padding: 12px 20px; font-size: 14px; margin: 5px; cursor: pointer; border: none; border-radius: 8px; color: white; font-weight: bold; }
    .blue { background: #007bff; } .green { background: #28a745; } .red { background: #dc3545; } .orange { background: #fd7e14; }
    input { padding: 10px; font-size: 16px; border-radius: 5px; border: 1px solid #ccc; width: 80%; margin-bottom: 10px; }
    h3 { margin-top: 20px; color: #333; border-top: 1px solid #eee; padding-top: 15px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Mechanical Clock</h2>
    <a href="/CLOCK"><button class="btn blue">REAL-TIME CLOCK</button></a>
    
    <h3>Stopwatch</h3>
    <a href="/SW_START"><button class="btn green">START</button></a>
    <a href="/SW_STOP"><button class="btn red">STOP</button></a>
    <a href="/SW_RESET"><button class="btn orange">RESET</button></a>

    <h3>Countdown Timer</h3>
    <form action="/SET_CD">
      <input type="number" name="m" placeholder="Minutes" min="0" max="99" required>
      <input type="number" name="s" placeholder="Seconds" min="0" max="59" required>
      <br>
      <button type="submit" class="btn blue">START COUNTDOWN</button>
    </form>

    <h3>Alarm Settings</h3>
    <form action="/SET_ALARM">
      <input type="time" name="atime" required>
      <br>
      <button type="submit" class="btn green">SET ALARM</button>
    </form>
    <a href="/OFF"><button class="btn red">STOP ALARM / RESET MODE</button></a>
  </div>
</body>
</html>
)=====";
