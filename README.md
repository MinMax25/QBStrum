# QBStrum
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](Resources/license.txt)
[![VST3](https://img.shields.io/badge/VST3-MIDI%20Effect-green)](https://steinbergmedia.github.io/vst3_dev_portal/)
[![Windows](https://img.shields.io/badge/Platform-Windows-blue)](#)
[![DAW](https://img.shields.io/badge/Tested%20DAW-Studio%20One-informational)](#)

DAWで使用する **ギター演奏特化型 MIDI エフェクトプラグイン** です。  

A **guitar-oriented MIDI effect plugin** for use in DAWs.

<img width="1300" height="800" alt="Image" src="Resources/Document/image/intro_pic_1.png" />

---

## 🎥 Demo Video

QBStrumの基本コンセプト、  
コード指定からストラム／アルペジオ演奏までの流れを動画で確認できます。

This video demonstrates the basic concept of QBStrum,  
including chord input and guitar-style performance generation.

[![QBStrum Demo Video](https://img.youtube.com/vi/XXXXXXXXXXX/0.jpg)](https://youtu.be/XXXXXXXXXXX)

---

## システム要件 | System Requirements

- **OS** : Windows  
- **Plugin Format** : VST3（MIDI Effect）  
- **DAW** : VST3 MIDI Effect に対応した DAW  

※ 音源は別途ギター音源が必要です。

---

- **OS** : Windows  
- **Plugin Format** : VST3 (MIDI Effect)  
- **DAW** : A DAW that supports VST3 MIDI effects  

※ A guitar instrument plugin is required separately.

---

## 概要 | Overview

QBStrum は、  
**コード指定を元にギターらしい演奏情報を生成する MIDI エフェクト**です。

ストローク、ブラッシング、アルペジオ、ミュート、  
アーティキュレーションといった  
**ギター奏法そのものを MIDI で制御**することを目的としています。

QBStrum 自体は音を出しません。  
演奏結果は **接続されたギター音源へ MIDI として送信**されます。

---

QBStrum is a **MIDI effect plugin that generates guitar-style performance data**  
based on chord input.

It focuses on controlling **playing techniques**, such as strumming, brushing,  
arpeggios, muting, and articulations, rather than sound generation.

QBStrum does not produce audio by itself.  
All generated MIDI data is sent to the connected guitar instrument plugin.

---

## 主な機能 | Features

- 🎸 **ギター奏法特化 MIDI 生成**
  - Up / Down ストラム
  - ブラシ奏法
  - アルペジオ（弦単位）
  - ミュート、デッドノート、ハンマリング等

- 🎼 **コード指定前提の設計**
  - DAW 側のコードトラックやイベントと連携
  - コードを直接 MIDI ノートで入力する必要なし

- 🎛 **弦・フレットを意識した制御**
  - 弦ごとの発音順・本数
  - 弦ごとのフレットオフセット
  - フレットノイズ生成（別 MIDI チャンネル）

- 🔌 **音源非依存**
  - 任意のギター音源で使用可能
  - KeySwitch / MIDI Note ベースの制御

---

- 🎸 **Guitar-oriented MIDI generation**
- 🎼 **Chord-based workflow**
- 🎛 **String- and fret-aware control**
- 🔌 **Instrument-independent design**

---

## コード指定について | About Chord Input

QBStrum は **DAW 側でのコード指定**を前提としています。

- コードトラック
- コードイベント
- Sound Variation / Expression 系機能

これらを通じて渡されたコード情報を内部で管理し、  
演奏生成に利用します。

QBStrum には独自のコード入力 UI はありません。  
これは制限ではなく、**DAW ごとのワークフローを尊重するための設計**です。

---

QBStrum relies on **chord input provided by the DAW**.

It does not include its own chord editor UI,  
allowing it to integrate naturally with each DAW’s workflow.

---

## ダウンロード・インストール | Download & Installation

### 手順 | Steps

1. このページ右側の **Releases** から最新版をダウンロード  
2. zip を任意のフォルダに展開  
3. 展開したフォルダ内の **VST3 プラグイン**を  
   DAW の VST3 プラグインフォルダへ配置

---

1. Download the latest version from **Releases**  
2. Extract the zip file  
3. Copy the **VST3 plugin** to your DAW’s VST3 plugin directory

---

## 対応・検証状況 | Compatibility Notes

QBStrum は **Studio One** を主な検証環境として開発されています。

ただし、  
特定の DAW 専用・推奨を目的としたものではありません。

DAW ごとの  
- コード指定方法  
- MIDI エフェクトの扱い  
- UI の思想  

によって挙動や使い勝手が異なる場合があります。

---

QBStrum is primarily tested with **Studio One**,  
but it is not intended to be DAW-exclusive.

Behavior may vary depending on how each DAW handles  
chord input and MIDI effects.

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
