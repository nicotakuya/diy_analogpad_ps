# diy_analogpad_ps

## overview
XE-1APの互換品を作る試みです。DUALSHOCK2をメガドライブに接続します。

メガドライブ版「アフターバーナーII」で動作を確認しています。

![diy_analogpadps](https://user-images.githubusercontent.com/5597377/174466662-26c29c60-1b4c-4b4d-b915-08dad657b230.jpg)

## 回路図

"diy_analogpadps_schematics.png"を参照

## 主な材料

・U1:Arduino Pro mini互換モジュール(ATmega328、5V動作、16MHz)

・U2:FT232RQ

・プレステ２用ゲームパッド「DUALSHOCK2」

・CN2:プレステ用コネクタ

・U3:3.3V三端子レギュレータ

・R1/2/3:1.5k ohm

・R4/5/6/7:3.3k ohm

## 作り方

Arduino IDEを使って、マイコンにプログラム(.ino)を書き込みます。

「アフターバーナーII」の場合、接続に成功すると、タイトル画面で「CONTROL ANALOG-JOY」と表示されます。

＊Arduinoのブートローダで時間が取り過ぎて、認識に失敗している可能性があります。その場合、メガドライブのリセットボタンを押すと改善します。

## movie
https://sites.google.com/site/yugenkaisyanico/diy-analogpad

## 関連
https://github.com/nicotakuya/diy_analogpad
