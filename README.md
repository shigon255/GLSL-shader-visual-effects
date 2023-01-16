# Final Project -- Implementing Visual effects using GLSL shader

<div align = "right">110550059 劉珆睿</div>

## How to build

+ 使用Visual Studio組建程式

+ 開啟Final_Project.sln後點選建置並執行即可開啟程式


## 實現效果

+ Skybox: 使用skybox將環境包圍，達到立體環境的效果
+ Shadow: 使用Shadow map實現shadow效果，可以移動光源查看效果，在隱形時shadow效果會關閉
+ Toon: 除了使用一般的Blinn-Phong將模型打光，也實作了toon shading效果
+ Frame & Blur: 使用OpenGL的stencil buffer將角色外圍描繪一層外框，並使用Gaussian filter將其模糊，目的是達到類似遊戲中強化的特效
+ Invisible: 將角色與環境融為一體，並將部分顏色blur達到較好的隱形效果

## How to manipulate

+ WASD與滑鼠控制相機視角
+ 上下左右控制光源，光源會環繞中心旋轉
+ 按R鍵可以切換Shading樣式: Blinn-phong <-> Toon
+ 按E鍵可以開關角色外框
+ 按Q鍵可以將切換角色外框樣式: 光暈 <-> 一般外框
+ 按T鍵可以開關角色隱形



