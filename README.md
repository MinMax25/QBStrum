# QBStrum
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](Resources/license.txt)
[![VST3](https://img.shields.io/badge/VST3-Steinberg-blueviolet)](https://www.steinberg.net/developers/)
[![Windows](https://img.shields.io/badge/OS-Windows-blue)](#)

Studio One（Fender Studio Pro）で使用する  
**ギター用ストラム／コード演奏支援 VST3 プラグイン**

A **MIDI-based guitar strumming / chord performance helper VST3 plugin**  
designed primarily for **Studio One / Fender Studio Pro**.

![QBStrum UI](resource/Document/Manual/image/QBStrum_Main_1.png)

---

## 概要 | Overview

QBStrum は、  
**コード指定 + ストラム／ブラッシング操作**を  
MIDI レベルで制御するギター演奏支援プラグインです。

QBStrum is a guitar performance helper plugin that controls  
**chord selection and strumming / brushing behavior** at the MIDI level.

コード、ボイシング、ストラム方向、ブラシ、ミュート、  
アルペジオ、アーティキュレーションなどを  
**KeySwitch とパラメータ操作**で統合的に扱えます。

Chords, voicings, strum direction, brushing, muting,  
arpeggios, and articulations are handled via  
**KeySwitches and parameter controls**.

※ 本プラグインは **MIDI 処理専用**であり、  
オーディオ信号は生成しません。

※ This plugin **does not generate audio**.  
A guitar virtual instrument is required.

---

## 対応環境 | System Requirements

- OS: **Windows**
- DAW: **VST3 対応 DAW**
  - 動作確認：Studio One 7 / Fender Studio Pro 8  
  - Verified: Studio One 7 / Fender Studio Pro 8
- ギター音源（別途必要）  
  - MIDI 入力対応音源（例：Ample Guitar）

- Guitar virtual instrument required  
  - Must support MIDI input (e.g. Ample Guitar series)

---

## ダウンロード・インストール | Download & Installation

### 手順 | Steps

1. このページ右側の **Releases** から最新版をダウンロードします。  
   Download the latest version from **Releases** on the right side of this page.

2. ダウンロードした zip を任意の場所に展開します。  
   Extract the downloaded zip file.

3. 展開されたフォルダ内の **QBStrum.vst3** を  
   フォルダごと VST3 プラグインフォルダへ配置します。

   Copy the entire **QBStrum.vst3** folder to your VST3 plugin directory.

#### VST3 フォルダ例 | Example (Windows)

C:\Program Files\Common Files\VST3


---

## 付属プリセットファイル | Included Preset Files

zip ファイルには `presets` フォルダ内に  
以下のファイルが含まれています。

The zip file includes the following files in the `presets` folder:

| File | Description |
|------|-------------|
| `default.qbs` | Default chord map (JSON format) |
| `QBStrum.pitchlist` | Pitch name definition (note / drum map display) |
| `QBStrum.keyswitch` | KeySwitch definition for chord control |

---

## プリセットファイルの役割と配置先 | Resource Files & Locations

### `default.qbs`
- 初回起動時に自動ロードされるコードマップ  
- ユーザー編集・差し替え可能  

- Automatically loaded chord map on first launch  
- User-editable

**配置先 | Location**
%USERPROFILE%\Documents\VST3 Presets\MinMax\QBStrum\ChordPreset

※ `%USERPROFILE%` はユーザーごとに異なります  
(e.g. `C:\Users\YourName`)

---

### `QBStrum.keyswitch`
- コード指定・切り替え用 KeySwitch 定義  
- Studio One 用

- KeySwitch definition for chord control  
- For Studio One

**配置先 | Location**  
Studio One ユーザープリセットフォルダ  
`Key Switches`

---

### `QBStrum.pitchlist`
- DAW 側のノート名／ドラムマップ表示用  
- Studio One 用

- Note / pitch name display for DAW  
- For Studio One

**配置先 | Location**  
Studio One ユーザープリセットフォルダ  
`Pitch Names`

---

## 注意事項 | Notes

- QBStrum は **Studio One / Fender Studio Pro** での使用を前提に設計されています。  
- 他 DAW でも動作する可能性はありますが、  
  付属リソースは正しく認識されない場合があります。

- QBStrum is designed primarily for **Studio One / Fender Studio Pro**.  
- Other DAWs may load the plugin, but included resource files may not function correctly.

---

## 免責事項 | Disclaimer

本ソフトウェアは無償で提供されます。  
本ソフトウェアの使用または使用不能から生じる  
いかなる損害についても、作者は一切の責任を負いません。  
本ソフトウェアは予告なく提供を中止することがあります。

This software is provided free of charge.  
The author assumes no responsibility for any damages  
arising from the use or inability to use this software.  
The software may be discontinued without notice.

---

## 📄 ライセンス | License

MIT License
