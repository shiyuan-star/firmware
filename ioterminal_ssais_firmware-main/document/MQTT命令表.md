

# MQTT指令介绍
- IoTerminal与外部系统通信通过MQTT协议进行，将通信的内容封装在JSON中。通过 JSON 中的 `control_type` 字段将指令分为五大类：

|  指令类型  | control_type区间 | 实例                          |
| :----: | :------------: | --------------------------- |
| 屏幕控件操作 |      1-49      | 设置屏幕文本框显示内容；屏幕按钮状态变化通知      |
| 屏幕状态操作 |     50-99      | 设置屏幕背光调到50% ；屏幕休眠通知         |
|  外设操作  |    100-149     | 设置三色灯以黄色间隔200ms闪烁；传感器数值上报   |
| 系统状态操作 |    150-199     | 设置ESP32到指定地址进行OTA更新 ；系统重启通知 |
|  业务逻辑  |    200-249     | 和具体业务相关的特定命令                |

# 屏幕控件操作指令（1-49）

**control_type类型一览表**

|   控件类型   | control_type字段值 |
| :------: | :-------------: |
|   控件属性   |        1        |
|    按钮    |       16        |
|    文本    |       17        |
|   进度条    |       18        |
|   滑动条    |       19        |
|    仪表    |       20        |
|   下拉列表   |       21        |
|    动画    |       22        |
|   时间显示   |       23        |
|  曲线图控件   |       24        |
| 表格数据记录控件 |       25        |
|   菜单控件   |       26        |
|   选择控件   |       27        |
|   二维码    |       28        |
|    图标    |       29        |



## 控件接收外部控制帧格式
### 控件属性
#### 设置控件隐藏
|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     控件类型     |       1        |
|   cmd_type   | 命令类型 | 设置控件隐藏：1 |
|  screen_id   |  控件所在屏幕ID  |     屏幕ID      |
|  control_id  |      控件ID      |     控件ID      |
|     hide     |   是否可见   | 隐藏：1 可见：0 |

``` JSON
{
	 "control_type": 1,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"hide":1
	 }
} 
```



### 按钮
#### 设置按钮状态

|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     控件类型     |       16        |
|   cmd_type   | 命令类型 | 修改按钮状态：1 |
|  screen_id   |  控件所在屏幕ID  |     屏幕ID      |
|  control_id  |      控件ID      |     控件ID      |
|     state     |   设置按键状态   | 按下：1 弹起：0 |

``` JSON
{
	 "control_type": 16,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"state":0
	 }
} 
```


### 文本
#### 设置文本控件显示字符

|    字段名    |     字段描述     |                   取值                   |
|:------------:|:----------------:|:----------------------------------------:|
| control_type |     控件类型     |                    17                    |
|   cmd_type   | 命令类型 |            修改显示字符串：1             |
|  screen_id   |  控件所在屏幕ID  |                  屏幕ID                  |
|  control_id  |      控件ID      |                  控件ID                  |
|    string    |     文本内容     | 字符串（长度取决于屏幕能显示的文本空间） |

``` JSON
{
	 "control_type":17,
	 "cmd_type":1,
	 "data":{
		"screen_id":1,
		"control_id":11,
		"string":"test"
	 }
}
```

#### 清除文本内容

|    字段名    |     字段描述     |                   取值                   |
|:------------:|:----------------:|:----------------------------------------:|
| control_type |     控件类型     |                    17                    |
|   cmd_type   | 命令类型 |            清除文本字符串：2             |
|  screen_id   |  控件所在屏幕ID  |                  屏幕ID                  |
|  control_id  |      控件ID      |                  控件ID                  |

``` JSON
{
	 "control_type":17,
	 "cmd_type":2,
	 "data":{
		"screen_id":0,
		"control_id":6,
	 }
}
```


#### 设置文本闪烁

|    字段名    |     字段描述     |                   取值                   |
|:------------:|:----------------:|:----------------------------------------:|
| control_type |     控件类型     |                    17                    |
|   cmd_type   | 命令类型 |            设置文本闪烁：3            |
|  screen_id   |  控件所在屏幕ID  |                  屏幕ID                  |
|  control_id  |      控件ID      |                  控件ID                  |
|    cycle    |     闪烁时间周期    | 单位：10ms,0表示不闪烁。 |

``` JSON
{
	 "control_type":17,
	 "cmd_type":3,
	 "data":{	 
		"screen_id":0,
		"control_id":6,
		"cycle":10
	 }
}
```


#### 设置文本滚动 

|    字段名    |     字段描述     |          取值           |
|:------------:|:----------------:|:-----------------------:|
| control_type |     控件类型     |           17            |
|   cmd_type   | 命令类型 |     设置文本滚动：4     |
|  screen_id   |  控件所在屏幕ID  |         屏幕ID          |
|  control_id  |      控件ID      |         控件ID          |
|  pixPerSec   | 每秒移动的像素点 | 单位：Pix,0表示不滚动。 |

``` JSON
{
	 "control_type":17,
	 "cmd_type":4,
	 "data":{	
		"screen_id":0,
		"control_id":6,
		"pixPerSec":50
	 }
}
```

#### 设置文本颜色

|    字段名    |     字段描述     |          取值           |
|:------------:|:----------------:|:-----------------------:|
| control_type |     控件类型     |           17            |
|   cmd_type   | 命令类型 |     设置文本颜色：5     |
|  screen_id   |  控件所在屏幕ID  |         屏幕ID          |
|  control_id  |      控件ID      |         控件ID          |
|  rgb   | 设置的文本颜色RGB888格式 | rgb888颜色值 |

``` JSON
{
	 "control_type":17,
	 "cmd_type":5,
	 "data":{	
		"screen_id":0,
		"control_id":6,
		"rgb":65535
	 }
}
```



#### 设置文本显示字符带颜色

|    字段名    |         字段描述         |      取值       |
|:------------:|:------------------------:|:---------------:|
| control_type |         控件类型         |       17        |
|   cmd_type   |         命令类型         | 设置文本显示字符带颜色：6 |
|  screen_id   |      控件所在屏幕ID      |     屏幕ID      |
|  control_id  |          控件ID          |     控件ID      |
|     rgb      | 设置的文本颜色RGB888格式 |  rgb888颜色值   |
|    string    |     文本内容     | 字符串（长度取决于屏幕能显示的文本空间） |

``` JSON
{
	 "control_type":17,
	 "cmd_type":6,
	 "data":{
		"screen_id":21,
		"control_id":10,
		"rgb":123456,
		"string":"test"
	 }
}
```



### 进度条
#### 设置进度条值

|    字段名    |     字段描述     |         取值         |
|:------------:|:----------------:|:--------------------:|
| control_type |     控件类型     |          18          |
|   cmd_type   | 命令类型 |   修改进度条值：1    |
|  screen_id   |  控件所在屏幕ID  |        屏幕ID        |
|  control_id  |      控件ID      |        控件ID        |
|    value     |    进度条取值    | 页面进度条的取值范围 |
``` JSON
{
	 "control_type": 18,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
	 	"control_id": 6,
		"value":50
	 }
} 
```

### 滑动条
#### 设置滑动条值

|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     控件类型     |       19        |
|   cmd_type   | 命令类型 | 修改滑动条值：1 |
|  screen_id   |  控件所在屏幕ID  |     屏幕ID      |
|  control_id  |      控件ID      |     控件ID      |
|    value     |    滑动条取值    | 页面滑动条的取值范围 |
``` JSON
{
	 "control_type": 19,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"value":40
	 }
} 
```

### 仪表
#### 设置仪表值

|    字段名    |     字段描述     |        取值        |
|:------------:|:----------------:|:------------------:|
| control_type |     控件类型     |         20         |
|   cmd_type   | 命令类型 |   修改仪表值：1    |
|  screen_id   |  控件所在屏幕ID  |       屏幕ID       |
|  control_id  |      控件ID      |       控件ID       |
|    value     |     仪表取值     | 页面仪表的取值范围 |
``` JSON
{
	 "control_type": 20,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"value":780
	 }
} 
```


### 动画
#### 播放控制

|    字段名    |     字段描述     |                 取值                 |
|:------------:|:----------------:|:------------------------------------:|
| control_type |     控件类型     |                  22                  |
|   cmd_type   | 命令类型 |             播放控制：1              |
|  screen_id   |  控件所在屏幕ID  |                屏幕ID                |
|  control_id  |      控件ID      |                控件ID                |
|    action    |   播放控制类型   | 开始播放：1 暂停播放：2  停止播放：3 |
``` JSON
{
	 "control_type": 22,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"action":1
	 }
} 
```


#### 播放指定帧

|    字段名    |     字段描述     |      取值      |
|:------------:|:----------------:|:--------------:|
| control_type |     控件类型     |       22       |
|   cmd_type   | 命令类型 | 播放指定帧：2  |
|  screen_id   |  控件所在屏幕ID  |     屏幕ID     |
|  control_id  |      控件ID      |     控件ID     |
|   frame_id   |     动画帧ID     | 帧ID(一个字节) |
``` JSON
{
	 "control_type": 22,
	 "cmd_type": 2,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"frame_id":123
	 }
} 
```

### 曲线图
#### 向指定曲线添加数据

|    字段名    |     字段描述     |          取值          |
|:------------:|:----------------:|:----------------------:|
| control_type |     控件类型     |           24           |
|   cmd_type   | 命令类型 |      添加数据：1       |
|  screen_id   |  控件所在屏幕ID  |         屏幕ID         |
|  control_id  |      控件ID      |         控件ID         |
|   channel    |     曲线通道     | 曲线在控件中的通道编号 |
|    pData     |      点数据      |         0-256          |
|   nDataLen   |      点个数      |    pData中数据个数     |
``` JSON
{
	 "control_type": 24,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"channel":1,
		"pointData": [3,25,57,124,255],
		"pointDataLen": 5
	 }
}
```

#### 删除曲线数据

|    字段名    |     字段描述     |          取值          |
|:------------:|:----------------:|:----------------------:|
| control_type |     控件类型     |           24           |
|   cmd_type   | 命令类型 |      删除曲线数据：2       |
|  screen_id   |  控件所在屏幕ID  |         屏幕ID         |
|  control_id  |      控件ID      |         控件ID         |
|   channel    |     曲线通道     | 曲线在控件中的通道编号 |
``` JSON
{
	 "control_type": 24,
	 "cmd_type": 2,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"channel":1
	 }
}
```

### 表格数据记录
#### 添加一行记录
|    字段名    |     字段描述     |                         取值                         |
|:------------:|:----------------:|:----------------------------------------------------:|
| control_type |     控件类型     |                          25                          |
|   cmd_type   | 命令类型 |                 添加一行常规记录：1                  |
|  screen_id   |  控件所在屏幕ID  |                        屏幕ID                        |
|  control_id  |      控件ID      |                        控件ID                        |
|    string    |     数据内容     | 不要超过表格列数，项目之间以";"隔开最后一项也要加";" |
``` JSON
{
	 "control_type": 25,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 40,
		"control_id": 3,
		"string":"Rotot1;storehouse;Painting station;42;"
	 }
}
```

#### 清除全部记录数据
|    字段名    |     字段描述     |        取值         |
|:------------:|:----------------:|:-------------------:|
| control_type |     控件类型     |         25          |
|   cmd_type   | 命令类型 | 清除全部记录数据：2 |
|  screen_id   |  控件所在屏幕ID  |       屏幕ID        |
|  control_id  |      控件ID      |       控件ID        |

``` JSON
{
	 "control_type": 25,
	 "cmd_type": 2,
	 "data":{
		"screen_id": 40,
	 	"control_id": 3,
	 }
}
```


#### 修改一条记录数据
|    字段名    | 字段描述         |                         取值                         |
|:------------:|:---------------- |:----------------------------------------------------:|
| control_type | 控件类型         |                          25                          |
|   cmd_type   | 命令类型 |                 修改一条记录数据：3                  |
|  screen_id   | 控件所在屏幕ID   |                        屏幕ID                        |
|  control_id  | 控件ID           |                        控件ID                        |
|   position   | 修改位置行号     |                第一行值为 0 往下递增                 |
|    string    | 数据内容         | 不要超过表格列数，项目之间以";"隔开最后一项也要加";" |

``` JSON
{
	 "control_type": 25,
	 "cmd_type": 3,
	 "data":{
		"screen_id": 40,
		"control_id": 3,
		"position":0,
		"string":"Rotot1;storehouse;Painting station;41;"
	 }
}
```

### 图标
#### 显示指定帧
|    字段名    |     字段描述     |      取值      |
|:------------:|:----------------:|:--------------:|
| control_type |     控件类型     |       29       |
|   cmd_type   | 命令类型 | 显示指定帧：1  |
|  screen_id   |  控件所在屏幕ID  |     屏幕ID     |
|  control_id  |      控件ID      |     控件ID     |
|   frame_id   |     图标帧ID     | 帧ID(一个字节) |

``` JSON
{
	 "control_type": 29,
	 "cmd_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"frame_id":123
	 }
} 
```


## 控件被触发向外发送通知
### 按钮
#### 按钮状态变化通知
|    字段名    |    字段描述    |      取值       |
|:------------:|:--------------:|:---------------:|
| control_type |    控件类型    |       16        |
| notify_type  |    通知类型    |   按钮状态：1   |
|  screen_id   | 控件所在屏幕ID |     屏幕ID      |
|  control_id  |     控件ID     |     控件ID      |
|    state     |    按键状态    | 按下：1 弹起：0 |

``` JSON
{
	 "control_type": 16,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"state":0
	 }
} 
```


### 文本
#### 文本输入通知
|    字段名    | 字段描述         |         取值         |
|:------------:|:---------------- |:--------------------:|
| control_type | 控件类型         |          17          |
| notify_type  | 通知类型         |     文本输入：1      |
|  screen_id   | 控件所在屏幕ID   |        屏幕ID        |
|  control_id  | 控件ID           |        控件ID        |
|    string    | 文本内容         | 屏幕输入的字符串内容 |
|     len      | 显示字符串的长度 |      字符串长度      |

``` JSON
{
	 "control_type": 17,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"string":"test",
		"len":5
	 }
} 
```

### 进度条
#### 进度条输入通知
|    字段名    |    字段描述    |         取值         |
|:------------:|:--------------:|:--------------------:|
| control_type |    控件类型    |          18          |
| notify_type  |    通知类型    |   修改进度条值：1    |
|  screen_id   | 控件所在屏幕ID |        屏幕ID        |
|  control_id  |     控件ID     |        控件ID        |
|    value     |   进度条取值   | 触摸位置的进度条取值 |

``` JSON
{
	 "control_type": 18,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"value":50
	 }
} 
```

### 滑动条
#### 滑动条输入通知

|    字段名    |    字段描述    |         取值         |
|:------------:|:--------------:|:--------------------:|
| control_type |    控件类型    |          19          |
| notify_type  |    通知类型    |   修改滑动条值：1    |
|  screen_id   | 控件所在屏幕ID |        屏幕ID        |
|  control_id  |     控件ID     |        控件ID        |
|    value     |   滑动条取值   | 触摸位置的滑动条取值 |

``` JSON
{
	 "control_type": 19,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"value":40
	 }
} 
```

#### 仪表输入通知

|    字段名    |    字段描述    |         取值         |
|:------------:|:--------------:|:--------------------:|
| control_type |    控件类型    |          20          |
| notify_type  |    通知类型    |   修改仪表值：1    |
|  screen_id   | 控件所在屏幕ID |        屏幕ID        |
|  control_id  |     控件ID     |        控件ID        |
|    value     |   滑动条取值   | 仪表取值 |

``` JSON
{
	 "control_type": 20,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"value":40
	 }
} 
```


### 菜单
#### 菜单状态通知

|    字段名    |    字段描述    |         取值         |
|:------------:|:--------------:|:--------------------:|
| control_type |    控件类型    |          26          |
| notify_type  |    通知类型    |   菜单状态通知：1    |
|  screen_id   | 控件所在屏幕ID |        屏幕ID        |
|  control_id  |     控件ID     |        控件ID        |
|     item     |   菜单项索引   | 触摸位置的菜单项索引 |
|    state     |    按钮状态    |   按下：1 松开：0    |

``` JSON
{
	 "control_type": 26,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"item":4,
		"state": 1
	 }
} 
```

### 选择控件
#### 选择类型通知

|    字段名    |    字段描述    |      取值       |
|:------------:|:--------------:|:---------------:|
| control_type |    控件类型    |       27        |
| notify_type  |    通知类型    | 选择类型通知：1 |
|  screen_id   | 控件所在屏幕ID |     屏幕ID      |
|  control_id  |     控件ID     |     控件ID      |
|     item     |      选项      | 触摸位置的选项  |

``` JSON
{
	 "control_type": 27,
	 "notify_type": 1,
	 "data":{
		"screen_id": 1,
		"control_id": 6,
		"item":4
	 }
} 
```

# 屏幕状态操作指令（50-99）
**control_type类型一览表**

|  屏幕状态类型  | control_type字段值 |
|:----------:|:------------------:|
|    未知    |         50         |
|  屏幕背光  |        51         |

## 屏幕状态接收外部数据帧格式

### 屏幕背光设置
####  息屏待机关闭背光
|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     屏幕状态类型     |       背光操作:51        |
|   cmd_type   | 命令类型 | 关闭背光：1 |

``` JSON
{
	 "control_type": 51,
	 "cmd_type": 1,
	 "data":{
	 }
} 
```


####  唤醒状态开启背光
|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     屏幕状态类型     |       背光操作:51        |
|   cmd_type   | 命令类型 | 开启背光：2 |

``` JSON
{
	 "control_type": 51,
	 "cmd_type": 2,
	 "data":{
	 }
} 
```


#### 自动背光调节（自动屏保）
|    字段名    |     字段描述     |    取值     |
|:------------:|:----------------:|:-----------:|
| control_type |     屏幕状态类型     |       背光操作:51        |
|   cmd_type   | 命令类型 | 开启背光：3 |
|   enable   |    背光自动调节使能    |   关：0 开：1   |
|   backLightOn   |    正常背光值    |   亮度值：0-255   |
|   backLightOff   |   省电背光值    |   亮度值：0-255   |
|   waitingTime   |   进入省电模式等待时间    |   3-65535 秒  |

``` JSON
{
    "control_type":51,
    "cmd_type":3,
    "data":{
        "enable":1,
        "backLightOn":250,
        "backLightOff":0,
        "waitingTime":60
    }
}
```
# 外设操作（100-149）
| 命令类型 | control_type字段值 |
| :--: | :-------------: |
| 三色灯  |       101       |
## 外设接收外部数据帧格式

### 三色灯
#### 控制三色灯状态
|     字段名      | 字段描述  | 字段类型 |             取值              |
| :----------: | :---: | :--: | :-------------------------: |
| control_type | 业务逻辑  | num  |         控制三色灯状态:101         |
|   cmd_type   | 命令类型  | num  |           下发状态：1            |
|    color     | 三色灯颜色 | num  | 范围（0-7） 按照RYG顺序，二进制标定颜色使能位。 |
|  beep_state  | 蜂鸣器状态 | num  |       停止鸣叫: 0   响鸣: 1       |

**color字段举例：**
  0 = RYG_000   表示  红灯，黄灯，绿灯全灭
  3 = RYG_011   表示  黄灯，绿灯亮，红灯灭
  5 = RYG_101   表示  红灯，绿灯亮，黄灯灭
  7 = RYG_111   表示  红灯，黄灯，绿灯全亮
  
``` JSON
// 绿灯亮 蜂鸣器停止
{
    "control_type":101,
    "cmd_type":1,
    "data":{
        "color":1,
        "beep_state":0
    }
}
```


# 系统状态操作指令（150-199）
**control_type类型一览表**

|  命令类型  | control_type字段值 |
| :----: | :-------------: |
|   未知   |       150       |
|   复位   |       151       |
|  OTA   |       152       |
| MQTT状态 |       153       |

## 系统状态接收外部数据帧格式
### 复位
#### 复位系统
|    字段名    |     字段描述     |      取值       |
|:------------:|:----------------:|:---------------:|
| control_type |     系统状态类型     |       复位:151        |
|   cmd_type   | 命令类型 | 复位ESP32：1 |

``` JSON
{
	 "control_type": 151,
	 "cmd_type": 1,
	 "data":{
	 }
} 
```

### OTA

#### 从NVS地址进行OTA更新
**注意：更新成功后系统会自动重启**

|    字段名    |   字段描述   |     取值     |
|:------------:|:------------:|:------------:|
| control_type | 系统状态类型 |   OTA:152   |
|   cmd_type   |   命令类型   | 从nvs地址OTA更新：1 |

``` JSON
{
	 "control_type": 152,
	 "cmd_type": 1,
	 "data":{
	 }
} 
```

#### 从指定地址进行OTA更新
**注意：更新成功后系统会自动重启**

|    字段名    |   字段描述   |     取值     |
|:------------:|:------------:|:------------:|
| control_type | 系统状态类型 |   OTA:152   |
|   cmd_type   |   命令类型   | 从指定地址进行OTA更新：2 |
| url             |    OTA服务器地址          |     http链接,将该字段设置为"default"将从NVS中存储的地址更新固件       |

``` JSON
{
	 "control_type": 152,
	 "cmd_type": 2,
	 "data":{
		 "url":"http://10.191.21.185:1880/ota/esp32/xxx"
	 }
} 
```

#### 查询NVS中存储的ESP32 OTA更新地址

|     字段名      |  字段描述  |            取值            |
| :----------: | :----: | :----------------------: |
| control_type | 系统状态类型 |         OTA:152          |
|   cmd_type   |  命令类型  | 查询NVS中存储的ESP32 OTA更新地址：3 |

``` JSON
{
	 "control_type": 152,
	 "cmd_type": 3,
	 "data":{
	 }
} 
```

#### 查询当前固件版本

|     字段名      |  字段描述  |    取值    |
| :----------: | :----: | :------: |
| control_type | 系统状态类型 | OTA:152  |
|   cmd_type   |  命令类型  | 查询固件版本：4 |

``` JSON
{
	 "control_type": 152,
	 "cmd_type": 4,
	 "data":{
	 }
} 
```

## 系统状态变化向外发送通知
### 复位
#### 系统复位重启通知
|     字段名      |  字段描述  |      取值       |
| :----------: | :----: | :-----------: |
| control_type | 系统状态类型 |    复位： 151    |
| notify_type  |  通知类型  |     重启：1      |
|   version    |  固件版本  | SS-AIS V0.1.4 |


``` JSON
{
	 "control_type": 151,
	 "notify_type": 1,
	 "data":{
		 "version":"SS-AIS V0.1.4"
	 }
} 
```

### OTA
#### 升级状态上报
|     字段名      |  字段描述   |                                        取值                                        |
| :----------: | :-----: | :------------------------------------------------------------------------------: |
| control_type | 系统状态类型  |                                     OTA： 152                                     |
| notify_type  |  通知类型   |                                    OTA结果上报：1                                     |
|    state     | OTA更新状态 | 开始：Download Firmware form: [url]  ；成功：SUCCEED System Restart  ；  失败：FAILED ；失败信息 |

``` JSON
{
	 "control_type": 152,
	 "notify_type": 1,
	 "data":{
		 "state":"Download Firmware form: http://192.168.88.108:1880/ss-ais/ota"
	 }
} 
```

#### NVS中存储的ESP32 OTA地址上报
|    字段名    |   字段描述   |      取值      |
|:------------:|:------------:|:--------------:|
| control_type | 系统状态类型 |   OTA： 152    |
| notify_type  |   通知类型   | NVS中存储的ESP32 OTA地址上报：2 |
| url             |     OTA服务器地址        | NVS存储的OTA服务器地址   |

``` JSON
{
	 "control_type": 152,
	 "notify_type": 2,
	 "data":{
		 "url":"http://10.191.21.185:1880/ota/esp32/xxx"
	 }
} 
```


#### 固件版本上报
|     字段名      |  字段描述  |      取值       |
| :----------: | :----: | :-----------: |
| control_type | 系统状态类型 |   OTA： 152    |
| notify_type  |  通知类型  |  当前的固件版本上报：4  |
|   version    |  固件版本  | SS-AIS V0.1.4 |

``` JSON
{
	 "control_type": 152,
	 "notify_type": 2,
	 "data":{
		 "version":"SS-AIS V0.1.4"
	 }
} 
```


### MQTT状态

#### MQTT掉线遗嘱

|     字段名      |  字段描述  |     取值      |
| :----------: | :----: | :---------: |
| control_type | 系统状态类型 | MQTT状态： 153 |
| notify_type  |  通知类型  | MQTT掉线遗嘱：1  |

``` JSON
{
	 "control_type": 153,
	 "notify_type": 1,
	 "data":{
	 }
} 
```

#### MQTT掉线重连次数报告

|     字段名      |  字段描述  |       取值       |
| :----------: | :----: | :------------: |
| control_type | 系统状态类型 |  MQTT状态： 153   |
| notify_type  |  通知类型  | MQTT掉线重连次数报告：2 |
|    count     |  重连次数  |    网络重连的次数     |

``` JSON
{
	 "control_type": 153,
	 "notify_type": 2,
	 "data":{
		 "count":3
	 }
} 
```

### 网络状态

#### 报告IP地址

|     字段名      |  字段描述  |     取值      |
| :----------: | :----: | :---------: |
| control_type | 系统状态类型 | 网络状态 154 |
| notify_type  |  通知类型  | 报告IP地址：1  |

``` JSON
{
	 "control_type": 154,
	 "notify_type": 1,
	 "data":{
		"ip" : "192.168.88.8"
	 }
} 
```


# 业务逻辑（200-249）
| 命令类型 | control_type字段值 |
| :--: | :-------------: |

