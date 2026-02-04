# QBStrum
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](Resources/license.txt)
[![VST3](https://img.shields.io/badge/VST3-Steinberg-blueviolet)](https://www.steinberg.net/developers/)
[![Windows](https://img.shields.io/badge/OS-Windows-blue)](#)

DAW（主に **Studio One**）で使用する  
**ギター用ストラム／コード演奏支援 VST3 プラグイン**です。

A **VST3 plugin** for guitar strumming and chord-based performance,  
designed primarily for **Studio One**.

---

## 概要 | Overview

QBStrum は、  
**コード指定 + ストラム／ブラッシング操作**を MIDI レベルで制御する  
ギター演奏支援プラグインです。

コード、ボイシング、ストラム方向、ブラシ、ミュート、  
アルペジオ、アーティキュレーションなどを  
**KeySwitch とパラメータ操作**で統合的に扱えます。

---

QBStrum is a guitar performance helper plugin that combines  
**chord specification and strumming control** at the MIDI level.

It supports chord voicings, strum directions, brushing, muting,  
arpeggios, and articulations via **KeySwitches and parameters**.

---

## 対応環境 | System Requirements

- OS: **Windows**
- DAW: **VST3 対応 DAW**
  - 動作確認：Studio One 7
- ギター音源（別途必要）
  - Ample Guitar などの MIDI 入力対応音源

---

- OS: **Windows**
- DAW: **VST3 compatible DAW**
  - Tested with Studio One 7
- Guitar instrument plugin required (not included)

---

## ダウンロード・インストール | Download & Installation

### 手順 | Steps

1. このページ右側の **Releases** から最新版をダウンロードします。  
2. ダウンロードした zip を任意の場所に展開します。  
3. 展開されたフォルダ内の **VST3 プラグイン**を  
   DAW の VST3 プラグインフォルダへ配置します。

---

1. Download the latest release from **Releases** on this page.  
2. Extract the downloaded zip file to any location.  
3. Copy the **VST3 plugin** to your DAW’s VST3 plugin directory.

---

## 付属リソースファイル | Resource Files

Release 用 zip には、`Resources` フォルダ内に  
以下の **3 つのリソースファイル**が含まれています。

These three resource files are included in the `Resources` folder.

| ファイル名 | 内容 |
|-----------|------|
| `default.qbs` | デフォルトでロードされる **コードマップ**（JSON 形式） |
| `QBStrum.pitchlist` | ピッチ名定義ファイル（ドラムマップ／ノート名表示用） |
| `QBStrum.keyswitch` | **コード指定用 KeySwitch 定義ファイル** |

---

| File | Description |
|------|-------------|
| `default.qbs` | Default chord map (JSON format) |
| `QBStrum.pitchlist` | Pitch name definition (for note naming / drum maps) |
| `QBStrum.keyswitch` | KeySwitch definition for chord input |

---

## リソース配置先 | Resource Installation

これらのファイルは、  
**QBStrum が参照する所定のフォルダ**へ配置する必要があります。

以下は一般的な配置例です。

---

These files must be placed in the **designated folder used by QBStrum**.  
A typical example is shown below.

### 例 | Example (Windows)

Documents/  
　└─ QBStrum/  
　　├─ default.qbs  
　　├─ QBStrum.pitchlist  
　　└─ QBStrum.keyswitch  


※ フォルダが存在しない場合は、手動で作成してください。

If the folder does not exist, please create it manually.

---

## リソースファイルの役割 | Notes

- `default.qbs`  
  - 初回起動時に **自動ロード**されるコードマップ  
  - ユーザー編集・差し替え可能  

- `QBStrum.pitchlist`  
  - DAW 側の **ノート名／ドラムマップ表示**に使用  

- `QBStrum.keyswitch`  
  - **コード指定・切り替え操作**用の KeySwitch 定義  

これらのファイルはすべて  
**ユーザーが自由に編集・拡張可能**です。

---

- `default.qbs`  
  - Automatically loaded on startup  
  - Fully user-editable  

- `QBStrum.pitchlist`  
  - Used for DAW note naming / drum maps  

- `QBStrum.keyswitch`  
  - Defines chord selection via KeySwitch  

All resource files can be freely edited or replaced.

---

## 注意事項 | Notes

- QBStrum は **特定 DAW 専用設計ではありません**が、  
  DAW ごとに KeySwitch やコード指定の扱いが異なる場合があります。
- 本プラグインは **MIDI 処理を主目的**としており、  
  オーディオ信号は生成しません。

---

- QBStrum is not strictly DAW-specific,  
  but behavior may vary depending on DAW implementation.
- This plugin processes **MIDI only** and does not generate audio.

---

## 免責事項 | Disclaimer

本ソフトウェアは無償で提供されます。  
本ソフトウェアの使用または使用不能から生じる  
いかなる損害についても、作者は一切の責任を負いません。

本ソフトウェアは予告なく提供を中止することがあります。

---

This software is provided **as is**, free of charge.  
The author assumes no responsibility for any damages  
resulting from the use or inability to use this software.

The software may be discontinued without prior notice.

---

## 📄 ライセンス | License

MIT License
