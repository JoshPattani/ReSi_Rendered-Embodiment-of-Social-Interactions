{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 5,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 1536.0, -287.0, 1886.0, 969.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"toolbars_unpinned_last_save" : 1,
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-130",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1383.077054977416992, 810.0, 223.0, 22.0 ],
					"text" : "OSC-route /userB_my_obci_EXG/bands"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-127",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 143.0, 810.0, 187.0, 22.0 ],
					"text" : "OSC-route /userA_my_obci_EXG"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-122",
					"linecount" : 2,
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ -4.538461685180664, 747.692378997802734, 50.0, 36.0 ],
					"text" : "0.440374"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-123",
					"linecount" : 2,
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 10.807693481445312, 550.769283294677734, 50.0, 36.0 ],
					"text" : "0.915514"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-124",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1.576923370361328, 489.230815887451172, 129.0, 22.0 ],
					"text" : "route /theta_beta_ratio"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-125",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 0.076923370361328, 684.615449905395508, 132.0, 22.0 ],
					"text" : "route /alpha_beta_ratio"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-121",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1226.07703971862793, 552.307744979858398, 50.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-120",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1236.923194885253906, 746.15391731262207, 50.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-116",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1227.692424774169922, 684.615449905395508, 132.0, 22.0 ],
					"text" : "route /aplha_beta_ratio"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-114",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1230.692424774169922, 489.230815887451172, 129.0, 22.0 ],
					"text" : "route /theta_beta_ratio"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-10",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 2115.384817123413086, 727.692377090454102, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-13",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 2072.307889938354492, 827.692386627197266, 189.0, 27.0 ],
					"text" : "Extra HRV Metrics"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-14",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2072.307889938354492, 632.30775260925293, 60.0, 22.0 ],
					"text" : "1357."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-17",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2158.46174430847168, 632.30775260925293, 125.0, 22.0 ],
					"text" : "OSC-route /hr/interval"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-20",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1755.384782791137695, 867.692390441894531, 69.0, 22.0 ],
					"text" : "15.797431"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-21",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1856.923254013061523, 881.538545608520508, 125.0, 22.0 ],
					"text" : "OSC-route /hrv/rmssd"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-22",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1755.384782791137695, 815.384693145751953, 69.0, 22.0 ],
					"text" : "50.80978"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-23",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1856.923254013061523, 823.077001571655273, 87.0, 36.0 ],
					"text" : "OSC-route /hrv/rmssdMax"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-24",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1481.538602828979492, 638.461599349975586, 78.0, 38.0 ],
					"text" : "Relax"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-25",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1392.307825088500977, 312.307722091674805, 278.0, 38.0 ],
					"text" : "Cyton"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-26",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1481.538602828979492, 441.538503646850586, 78.0, 38.0 ],
					"text" : "Focus"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-27",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1547.692455291748047, 718.461606979370117, 47.0, 27.0 ],
					"text" : "MAX"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-28",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1550.769378662109375, 526.153896331787109, 47.0, 27.0 ],
					"text" : "MAX"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-29",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1458.461677551269531, 718.461606979370117, 42.0, 27.0 ],
					"text" : "MIN"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-30",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1618.461692810058594, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.301676"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-31",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1552.307840347290039, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.001735"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-32",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1486.153987884521484, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.000764"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-33",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1420.00013542175293, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.001589"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-34",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1353.846282958984375, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.694236"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-35",
					"maxclass" : "newobj",
					"numinlets" : 6,
					"numoutlets" : 6,
					"outlettype" : [ "", "", "", "", "", "" ],
					"patching_rect" : [ 1383.077054977416992, 889.230854034423828, 224.0, 22.0 ],
					"text" : "route /delta /theta /alpha /beta /gamma"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-37",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1458.461677551269531, 526.153896331787109, 42.0, 27.0 ],
					"text" : "MIN"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-39",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1546.153993606567383, 752.307764053344727, 60.0, 22.0 ],
					"text" : "0.8867"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-40",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1483.077064514160156, 786.153921127319336, 61.0, 22.0 ],
					"text" : "0.796696"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-41",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1458.461677551269531, 752.307764053344727, 60.0, 22.0 ],
					"text" : "0.15638"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-42",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 1461.538600921630859, 684.615449905395508, 208.0, 22.0 ],
					"text" : "route /relaxMin /restfulness /relaxMax"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-43",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1552.307840347290039, 553.846206665039062, 61.0, 22.0 ],
					"text" : "0.992037"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-44",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1486.153987884521484, 586.153902053833008, 60.0, 22.0 ],
					"text" : "0.681298"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-46",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1458.461677551269531, 553.846206665039062, 64.0, 22.0 ],
					"text" : "0.588012"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-47",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 1461.538600921630859, 489.230815887451172, 219.0, 22.0 ],
					"text" : "route /focusMin /mindfulness /focusMax"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-48",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1383.077054977416992, 407.692346572875977, 229.0, 22.0 ],
					"text" : "OSC-route /userB_my_obci_EXG/metrics"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-49",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1690.769392013549805, 260.000024795532227, 189.0, 27.0 ],
					"text" : "User B"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-51",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1989.230958938598633, 312.307722091674805, 278.0, 38.0 ],
					"text" : "Arduino"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-52",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1769.230937957763672, 413.846193313598633, 189.0, 27.0 ],
					"text" : "GSR Sweat Sensor"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-53",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 1755.384782791137695, 718.461606979370117, 202.0, 27.0 ],
					"text" : "Heart Rate Variability"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-54",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 2070.769428253173828, 675.384679794311523, 165.0, 27.0 ],
					"text" : "PPG Heart Beat Pulse"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-55",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 2072.307889938354492, 441.538503646850586, 189.0, 27.0 ],
					"text" : "PPG Heart Rate"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-56",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2070.769428253173828, 729.230838775634766, 24.0, 22.0 ],
					"text" : "0."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-57",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2158.46174430847168, 729.230838775634766, 103.0, 22.0 ],
					"text" : "OSC-route /pulse"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-58",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2072.307889938354492, 929.230857849121094, 66.5, 22.0 ],
					"text" : "38.78783"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-59",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2156.923282623291016, 929.230857849121094, 125.0, 22.0 ],
					"text" : "OSC-route /hrv/sdann"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-60",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2072.307889938354492, 881.538545608520508, 66.0, 22.0 ],
					"text" : "46.46619"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-61",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2156.923282623291016, 889.230854034423828, 119.0, 22.0 ],
					"text" : "OSC-route /hrv/sdnn"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-62",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1755.384782791137695, 763.076995849609375, 69.0, 22.0 ],
					"text" : "8.305586"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-63",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1856.923254013061523, 763.076995849609375, 84.0, 36.0 ],
					"text" : "OSC-route /hrv/rmssdMin"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-75",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 1836.923252105712891, 493.846200942993164, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-78",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1769.230937957763672, 464.615428924560547, 50.0, 22.0 ],
					"text" : "357."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-89",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1876.923255920410156, 464.615428924560547, 112.0, 22.0 ],
					"text" : "OSC-route /gsr/min"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-96",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2018.46173095703125, 407.692346572875977, 113.0, 22.0 ],
					"text" : "OSC-route /User_B"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-97",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 1835.384790420532227, 552.307744979858398, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-101",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1769.230937957763672, 526.153896331787109, 50.0, 22.0 ],
					"text" : "606."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-102",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1875.384794235229492, 526.153896331787109, 115.0, 22.0 ],
					"text" : "OSC-route /gsr/max"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-103",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 1833.846328735351562, 610.769289016723633, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-104",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2072.307889938354492, 587.692363739013672, 60.0, 22.0 ],
					"text" : "76."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-105",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2158.46174430847168, 587.692363739013672, 115.0, 22.0 ],
					"text" : "OSC-route /hr/value"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-107",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2073.846351623535156, 541.53851318359375, 57.0, 22.0 ],
					"text" : "107."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-109",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2158.46174430847168, 541.53851318359375, 109.0, 22.0 ],
					"text" : "OSC-route /hr/max"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-110",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1769.230937957763672, 586.153902053833008, 50.0, 22.0 ],
					"text" : "599."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-111",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 1872.307870864868164, 586.153902053833008, 121.0, 22.0 ],
					"text" : "OSC-route /gsr/value"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-112",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 2072.307889938354492, 493.846200942993164, 60.0, 22.0 ],
					"text" : "61."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-113",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 2158.46174430847168, 493.846200942993164, 106.0, 22.0 ],
					"text" : "OSC-route /hr/min"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-16",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 876.923160552978516, 727.692377090454102, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-11",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 833.846233367919922, 827.692386627197266, 189.0, 27.0 ],
					"text" : "Extra HRV Metrics"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 833.846233367919922, 632.30775260925293, 60.0, 22.0 ],
					"text" : "1048."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-9",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 920.000087738037109, 632.30775260925293, 125.0, 22.0 ],
					"text" : "OSC-route /hr/interval"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-164",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 516.923126220703125, 869.230852127075195, 69.0, 22.0 ],
					"text" : "52.558136"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-165",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 620.000059127807617, 883.077007293701172, 125.0, 22.0 ],
					"text" : "OSC-route /hrv/rmssd"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-162",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 516.923126220703125, 816.923154830932617, 69.0, 22.0 ],
					"text" : "52.558136"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-163",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 620.000059127807617, 823.077001571655273, 87.0, 36.0 ],
					"text" : "OSC-route /hrv/rmssdMax"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-161",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 244.615407943725586, 640.00006103515625, 78.0, 38.0 ],
					"text" : "Relax"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-160",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 153.846168518066406, 313.846183776855469, 278.0, 38.0 ],
					"text" : "Cyton"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-159",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 244.615407943725586, 441.538503646850586, 78.0, 38.0 ],
					"text" : "Focus"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-158",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 310.769260406494141, 720.000068664550781, 47.0, 27.0 ],
					"text" : "MAX"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-157",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 313.846183776855469, 526.153896331787109, 47.0, 27.0 ],
					"text" : "MAX"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-156",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 220.000020980834961, 720.000068664550781, 42.0, 27.0 ],
					"text" : "MIN"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-117",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 380.000036239624023, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.301676"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-118",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 313.846183776855469, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.001735"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-115",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 249.230792999267578, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.000764"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-108",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 181.538478851318359, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.001589"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-106",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 115.384626388549805, 941.538551330566406, 59.0, 22.0 ],
					"text" : "0.694236"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-100",
					"maxclass" : "newobj",
					"numinlets" : 6,
					"numoutlets" : 6,
					"outlettype" : [ "", "", "", "", "", "" ],
					"patching_rect" : [ 146.153860092163086, 889.230854034423828, 224.0, 22.0 ],
					"text" : "route /delta /theta /alpha /beta /gamma"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-99",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 146.153860092163086, 850.769311904907227, 105.0, 22.0 ],
					"text" : "OSC-route /bands"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-98",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 220.000020980834961, 526.153896331787109, 42.0, 27.0 ],
					"text" : "MIN"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-73",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 309.230798721313477, 752.307764053344727, 60.0, 22.0 ],
					"text" : "0.885912"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-74",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 244.615407943725586, 786.153921127319336, 61.0, 22.0 ],
					"text" : "0.110095"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-87",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 220.000020980834961, 752.307764053344727, 60.0, 22.0 ],
					"text" : "0.03195"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-50",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 224.615406036376953, 684.615449905395508, 208.0, 22.0 ],
					"text" : "route /relaxMin /restfulness /relaxMax"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-15",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 315.384645462036133, 553.846206665039062, 61.0, 22.0 ],
					"text" : "0.98306"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-12",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 249.230792999267578, 586.153902053833008, 60.0, 22.0 ],
					"text" : "0.741216"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-7",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 220.000020980834961, 553.846206665039062, 64.0, 22.0 ],
					"text" : "0.408475"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 4,
					"outlettype" : [ "", "", "", "" ],
					"patching_rect" : [ 224.615406036376953, 489.230815887451172, 219.0, 22.0 ],
					"text" : "route /focusMin /mindfulness /focusMax"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-19",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 146.153860092163086, 407.692346572875977, 229.0, 22.0 ],
					"text" : "OSC-route /userA_my_obci_EXG/metrics"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 1,
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-95",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 492.3077392578125, 227.692329406738281, 189.0, 27.0 ],
					"text" : "User A"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 24.0,
					"id" : "obj-94",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 750.769302368164062, 313.846183776855469, 278.0, 38.0 ],
					"text" : "Arduino"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-93",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 532.307743072509766, 413.846193313598633, 189.0, 27.0 ],
					"text" : "GSR Sweat Sensor"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-92",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 516.923126220703125, 720.000068664550781, 202.0, 27.0 ],
					"text" : "Heart Rate Variability"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-91",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 833.846233367919922, 675.384679794311523, 165.0, 27.0 ],
					"text" : "PPG Heart Beat Pulse"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Hanken Grotesk",
					"fontsize" : 16.0,
					"id" : "obj-90",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 833.846233367919922, 441.538503646850586, 189.0, 27.0 ],
					"text" : "PPG Heart Rate"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-88",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 833.846233367919922, 729.230838775634766, 24.0, 22.0 ],
					"text" : "0."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-86",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 920.000087738037109, 729.230838775634766, 103.0, 22.0 ],
					"text" : "OSC-route /pulse"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-80",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 833.846233367919922, 929.230857849121094, 66.5, 22.0 ],
					"text" : "15.696611"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-81",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 918.461626052856445, 929.230857849121094, 125.0, 22.0 ],
					"text" : "OSC-route /hrv/sdann"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-82",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 835.384695053100586, 883.077007293701172, 66.0, 22.0 ],
					"text" : "39.827751"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-83",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 918.461626052856445, 889.230854034423828, 119.0, 22.0 ],
					"text" : "OSC-route /hrv/sdnn"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-84",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 516.923126220703125, 763.076995849609375, 69.0, 22.0 ],
					"text" : "12.20202"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-85",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 620.000059127807617, 763.076995849609375, 84.0, 36.0 ],
					"text" : "OSC-route /hrv/rmssdMin"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-76",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 598.46159553527832, 493.846200942993164, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-77",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 532.307743072509766, 466.153890609741211, 50.0, 22.0 ],
					"text" : "381."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-79",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 640.00006103515625, 466.153890609741211, 112.0, 22.0 ],
					"text" : "OSC-route /gsr/min"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-72",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 780.00007438659668, 407.692346572875977, 113.0, 22.0 ],
					"text" : "OSC-route /User_A"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-65",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 598.46159553527832, 552.307744979858398, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-67",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 532.307743072509766, 526.153896331787109, 50.0, 22.0 ],
					"text" : "949."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-70",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 638.461599349975586, 526.153896331787109, 115.0, 22.0 ],
					"text" : "OSC-route /gsr/max"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-64",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 596.923133850097656, 610.769289016723633, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-71",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 833.846233367919922, 589.230825424194336, 60.0, 22.0 ],
					"text" : "87."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-69",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 920.000087738037109, 589.230825424194336, 115.0, 22.0 ],
					"text" : "OSC-route /hr/value"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-68",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 836.92315673828125, 543.076974868774414, 57.0, 22.0 ],
					"text" : "92."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-66",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 920.000087738037109, 543.076974868774414, 109.0, 22.0 ],
					"text" : "OSC-route /hr/max"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-45",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 1110.769336700439453, 143.076936721801758, 24.0, 24.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 532.307743072509766, 586.153902053833008, 50.0, 22.0 ],
					"text" : "947."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-18",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 633.846214294433594, 586.153902053833008, 121.0, 22.0 ],
					"text" : "OSC-route /gsr/value"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.815686274509804, 0.309803921568627, 0.309803921568627, 0.0 ],
					"id" : "obj-38",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 15.384616851806641, 15.384616851806641, 238.0, 62.0 ],
					"presentation" : 1,
					"presentation_linecount" : 4,
					"presentation_rect" : [ 418.4375, 105.5, 238.0, 62.0 ],
					"text" : "Wireless Sensor Communication \nwith OpenBCI and Arduino Cloud\n\nRESI, February 2025",
					"textcolor" : [ 0.996078431372549, 0.996078431372549, 0.996078431372549, 1.0 ],
					"textjustification" : 1
				}

			}
, 			{
				"box" : 				{
					"angle" : 270.0,
					"bgcolor" : [ 0.150563135743141, 0.150558635592461, 0.150561213493347, 1.0 ],
					"id" : "obj-6",
					"maxclass" : "panel",
					"mode" : 0,
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 10.769231796264648, 9.230770111083984, 248.0, 78.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 412.875, 99.0, 248.0, 78.0 ],
					"proportion" : 0.5
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 833.846233367919922, 493.846200942993164, 60.0, 22.0 ],
					"text" : "61."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 920.000087738037109, 493.846200942993164, 106.0, 22.0 ],
					"text" : "OSC-route /hr/min"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 1041.53856086730957, 95.384624481201172, 104.0, 22.0 ],
					"text" : "udpreceive 42069"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-12", 1 ],
					"source" : [ "obj-1", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 1 ],
					"source" : [ "obj-1", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-7", 1 ],
					"midpoints" : [ 234.115406036376953, 524.278612852096558, 274.500020980834961, 524.278612852096558 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-106", 1 ],
					"midpoints" : [ 155.653860092163086, 928.461538314819336, 164.884626388549805, 928.461538314819336 ],
					"source" : [ "obj-100", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-108", 1 ],
					"midpoints" : [ 196.653860092163086, 928.461538314819336, 231.038478851318359, 928.461538314819336 ],
					"source" : [ "obj-100", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-115", 1 ],
					"midpoints" : [ 237.653860092163086, 928.461538314819336, 298.730792999267578, 928.461538314819336 ],
					"source" : [ "obj-100", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-117", 1 ],
					"midpoints" : [ 319.653860092163086, 928.461538314819336, 429.500036239624023, 928.461538314819336 ],
					"source" : [ "obj-100", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-118", 1 ],
					"midpoints" : [ 278.653860092163086, 928.461538314819336, 363.346183776855469, 928.461538314819336 ],
					"source" : [ "obj-100", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-101", 1 ],
					"midpoints" : [ 1884.884794235229492, 556.041319608688354, 1854.784911870956421, 556.041319608688354, 1854.784911870956421, 529.041319608688354, 1809.730937957763672, 529.041319608688354 ],
					"order" : 1,
					"source" : [ "obj-102", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-97", 0 ],
					"midpoints" : [ 1884.884794235229492, 556.041319608688354, 1854.784911870956421, 556.041319608688354, 1854.784911870956421, 544.041319608688354, 1844.884790420532227, 544.041319608688354 ],
					"order" : 0,
					"source" : [ "obj-102", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-104", 1 ],
					"midpoints" : [ 2167.96174430847168, 614.803906027884523, 2142.870314359664917, 614.803906027884523, 2142.870314359664917, 581.134672785647467, 2122.807889938354492, 581.134672785647467 ],
					"source" : [ "obj-105", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-107", 1 ],
					"midpoints" : [ 2167.96174430847168, 571.033902812976407, 2142.870314359664917, 571.033902812976407, 2142.870314359664917, 537.364669570739352, 2121.346351623535156, 537.364669570739352 ],
					"source" : [ "obj-109", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-103", 0 ],
					"midpoints" : [ 1881.807870864868164, 610.041319608688354, 1845.784911870956421, 610.041319608688354, 1845.784911870956421, 604.041319608688354, 1843.346328735351562, 604.041319608688354 ],
					"order" : 0,
					"source" : [ "obj-111", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-110", 1 ],
					"midpoints" : [ 1881.807870864868164, 610.041319608688354, 1854.784911870956421, 610.041319608688354, 1854.784911870956421, 592.041319608688354, 1818.784911870956421, 592.041319608688354, 1818.784911870956421, 580.041319608688354, 1809.730937957763672, 580.041319608688354 ],
					"order" : 1,
					"source" : [ "obj-111", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-112", 1 ],
					"midpoints" : [ 2167.96174430847168, 520.530052949620881, 2142.870314359664917, 520.530052949620881, 2142.870314359664917, 490.227743031607588, 2122.807889938354492, 490.227743031607588 ],
					"source" : [ "obj-113", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-121", 1 ],
					"source" : [ "obj-114", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-120", 1 ],
					"source" : [ "obj-116", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-123", 1 ],
					"source" : [ "obj-124", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-122", 1 ],
					"source" : [ "obj-125", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-99", 0 ],
					"source" : [ "obj-127", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-35", 0 ],
					"source" : [ "obj-130", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-162", 1 ],
					"midpoints" : [ 629.500059127807617, 855.461538314819336, 599.743250855859742, 855.461538314819336, 599.743250855859742, 807.461538314819336, 576.423126220703125, 807.461538314819336 ],
					"source" : [ "obj-163", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-164", 1 ],
					"midpoints" : [ 629.500059127807617, 907.461538314819336, 599.743250855859742, 907.461538314819336, 599.743250855859742, 865.461538314819336, 576.423126220703125, 865.461538314819336 ],
					"source" : [ "obj-165", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-14", 1 ],
					"midpoints" : [ 2167.96174430847168, 658.875030368857097, 2142.870314359664917, 658.875030368857097, 2142.870314359664917, 625.205797126620041, 2122.807889938354492, 625.205797126620041 ],
					"source" : [ "obj-17", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 1 ],
					"midpoints" : [ 643.346214294433594, 610.656656980514526, 616.938497304916382, 610.656656980514526, 616.938497304916382, 592.656656980514526, 580.938497304916382, 592.656656980514526, 580.938497304916382, 580.656656980514526, 572.807743072509766, 580.656656980514526 ],
					"order" : 1,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-64", 0 ],
					"midpoints" : [ 643.346214294433594, 610.656656980514526, 607.938497304916382, 610.656656980514526, 607.938497304916382, 604.656656980514526, 606.423133850097656, 604.656656980514526 ],
					"order" : 0,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"midpoints" : [ 155.653860092163086, 475.461538314819336, 234.115406036376953, 475.461538314819336 ],
					"order" : 1,
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-124", 0 ],
					"midpoints" : [ 155.653860092163086, 475.076927185058594, 11.076923370361328, 475.076927185058594 ],
					"order" : 2,
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-125", 0 ],
					"midpoints" : [ 155.653860092163086, 613.000009536743164, 9.576923370361328, 613.000009536743164 ],
					"order" : 3,
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-50", 0 ],
					"midpoints" : [ 155.653860092163086, 610.461538314819336, 234.115406036376953, 610.461538314819336 ],
					"order" : 0,
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-127", 0 ],
					"midpoints" : [ 1051.03856086730957, 213.0, 453.0, 213.0, 453.0, 627.0, 152.5, 627.0 ],
					"order" : 6,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-130", 0 ],
					"midpoints" : [ 1051.03856086730957, 225.0, 1212.0, 225.0, 1212.0, 795.0, 1392.577054977416992, 795.0 ],
					"order" : 1,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-19", 0 ],
					"midpoints" : [ 1051.03856086730957, 386.769229888916016, 155.653860092163086, 386.769229888916016 ],
					"order" : 5,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"midpoints" : [ 1051.03856086730957, 128.538463592529297, 1120.269336700439453, 128.538463592529297 ],
					"order" : 3,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-48", 0 ],
					"midpoints" : [ 1051.03856086730957, 385.30769157409668, 1392.577054977416992, 385.30769157409668 ],
					"order" : 2,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-72", 0 ],
					"midpoints" : [ 1051.03856086730957, 386.769229888916016, 789.50007438659668, 386.769229888916016 ],
					"order" : 4,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-96", 0 ],
					"midpoints" : [ 1051.03856086730957, 385.30769157409668, 2027.96173095703125, 385.30769157409668 ],
					"order" : 0,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 1 ],
					"midpoints" : [ 1866.423254013061523, 906.846200942993164, 1837.589665421899554, 906.846200942993164, 1837.589665421899554, 864.846200942993164, 1814.884782791137695, 864.846200942993164 ],
					"source" : [ "obj-21", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-22", 1 ],
					"midpoints" : [ 1866.423254013061523, 854.846200942993164, 1837.589665421899554, 854.846200942993164, 1837.589665421899554, 806.846200942993164, 1814.884782791137695, 806.846200942993164 ],
					"source" : [ "obj-23", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-8", 1 ],
					"midpoints" : [ 929.500087738037109, 521.145390321447053, 905.023899793624878, 521.145390321447053, 905.023899793624878, 490.84308040343376, 884.346233367919922, 490.84308040343376 ],
					"source" : [ "obj-3", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-30", 1 ],
					"midpoints" : [ 1556.577054977416992, 927.846200942993164, 1667.961692810058594, 927.846200942993164 ],
					"source" : [ "obj-35", 4 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 1 ],
					"midpoints" : [ 1515.577054977416992, 927.846200942993164, 1601.807840347290039, 927.846200942993164 ],
					"source" : [ "obj-35", 3 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 1 ],
					"midpoints" : [ 1474.577054977416992, 927.846200942993164, 1535.653987884521484, 927.846200942993164 ],
					"source" : [ "obj-35", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 1 ],
					"midpoints" : [ 1433.577054977416992, 927.846200942993164, 1469.50013542175293, 927.846200942993164 ],
					"source" : [ "obj-35", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-34", 1 ],
					"midpoints" : [ 1392.577054977416992, 927.846200942993164, 1403.346282958984375, 927.846200942993164 ],
					"source" : [ "obj-35", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-39", 1 ],
					"source" : [ "obj-42", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-40", 1 ],
					"source" : [ "obj-42", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-41", 1 ],
					"midpoints" : [ 1471.038600921630859, 713.846200942993164, 1508.46174430847168, 713.846200942993164, 1508.46174430847168, 746.846200942993164, 1508.961677551269531, 746.846200942993164 ],
					"source" : [ "obj-42", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-43", 1 ],
					"source" : [ "obj-47", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-44", 1 ],
					"source" : [ "obj-47", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 1 ],
					"midpoints" : [ 1471.038600921630859, 523.663275480270386, 1512.961677551269531, 523.663275480270386 ],
					"source" : [ "obj-47", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-114", 0 ],
					"midpoints" : [ 1392.577054977416992, 472.615379333496094, 1240.192424774169922, 472.615379333496094 ],
					"order" : 2,
					"source" : [ "obj-48", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-116", 0 ],
					"midpoints" : [ 1392.577054977416992, 613.615379333496094, 1237.192424774169922, 613.615379333496094 ],
					"order" : 3,
					"source" : [ "obj-48", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-42", 0 ],
					"midpoints" : [ 1392.577054977416992, 609.846200942993164, 1471.038600921630859, 609.846200942993164 ],
					"order" : 0,
					"source" : [ "obj-48", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-47", 0 ],
					"midpoints" : [ 1392.577054977416992, 474.846200942993164, 1471.038600921630859, 474.846200942993164 ],
					"order" : 1,
					"source" : [ "obj-48", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-73", 1 ],
					"source" : [ "obj-50", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-74", 1 ],
					"source" : [ "obj-50", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-87", 1 ],
					"midpoints" : [ 234.115406036376953, 714.461538314819336, 270.615329742431641, 714.461538314819336, 270.615329742431641, 747.461538314819336, 270.500020980834961, 747.461538314819336 ],
					"source" : [ "obj-50", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-10", 0 ],
					"midpoints" : [ 2167.96174430847168, 753.846200942993164, 2138.46174430847168, 753.846200942993164, 2138.46174430847168, 723.846200942993164, 2124.884817123413086, 723.846200942993164 ],
					"order" : 0,
					"source" : [ "obj-57", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-56", 1 ],
					"midpoints" : [ 2167.96174430847168, 761.846200942993164, 2096.46174430847168, 761.846200942993164, 2096.46174430847168, 723.846200942993164, 2085.269428253173828, 723.846200942993164 ],
					"order" : 1,
					"source" : [ "obj-57", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-58", 1 ],
					"midpoints" : [ 2166.423282623291016, 953.358395338058472, 2140.016657829284668, 953.358395338058472, 2140.016657829284668, 923.358395338058472, 2129.307889938354492, 923.358395338058472 ],
					"source" : [ "obj-59", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-60", 1 ],
					"midpoints" : [ 2166.423282623291016, 911.846200942993164, 2139.96174430847168, 911.846200942993164, 2139.96174430847168, 875.846200942993164, 2128.807889938354492, 875.846200942993164 ],
					"source" : [ "obj-61", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-62", 1 ],
					"midpoints" : [ 1866.423254013061523, 800.846200942993164, 1837.589665421899554, 800.846200942993164, 1837.589665421899554, 758.846200942993164, 1814.884782791137695, 758.846200942993164 ],
					"source" : [ "obj-63", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-68", 1 ],
					"midpoints" : [ 929.500087738037109, 571.649240184802579, 905.023899793624878, 571.649240184802579, 905.023899793624878, 537.980006942565524, 884.42315673828125, 537.980006942565524 ],
					"source" : [ "obj-66", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-71", 1 ],
					"midpoints" : [ 929.500087738037109, 615.419243399710695, 905.023899793624878, 615.419243399710695, 905.023899793624878, 581.750010157473639, 884.346233367919922, 581.750010157473639 ],
					"source" : [ "obj-69", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-65", 0 ],
					"midpoints" : [ 647.961599349975586, 556.656656980514526, 616.938497304916382, 556.656656980514526, 616.938497304916382, 544.656656980514526, 607.96159553527832, 544.656656980514526 ],
					"order" : 0,
					"source" : [ "obj-70", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-67", 1 ],
					"midpoints" : [ 647.961599349975586, 556.656656980514526, 616.938497304916382, 556.656656980514526, 616.938497304916382, 529.656656980514526, 572.807743072509766, 529.656656980514526 ],
					"order" : 1,
					"source" : [ "obj-70", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-163", 0 ],
					"midpoints" : [ 789.50007438659668, 809.461538314819336, 629.500059127807617, 809.461538314819336 ],
					"order" : 11,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-165", 0 ],
					"midpoints" : [ 789.50007438659668, 865.461538314819336, 629.500059127807617, 865.461538314819336 ],
					"order" : 10,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-18", 0 ],
					"midpoints" : [ 789.50007438659668, 571.656656980514526, 643.346214294433594, 571.656656980514526 ],
					"order" : 9,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-3", 0 ],
					"midpoints" : [ 789.50007438659668, 475.461538314819336, 929.500087738037109, 475.461538314819336 ],
					"order" : 4,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-66", 0 ],
					"midpoints" : [ 789.50007438659668, 526.461538314819336, 929.500087738037109, 526.461538314819336 ],
					"order" : 3,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-69", 0 ],
					"midpoints" : [ 789.50007438659668, 575.461538314819336, 929.500087738037109, 575.461538314819336 ],
					"order" : 2,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-70", 0 ],
					"midpoints" : [ 789.50007438659668, 520.656656980514526, 647.961599349975586, 520.656656980514526 ],
					"order" : 8,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-79", 0 ],
					"midpoints" : [ 789.50007438659668, 451.437139987945557, 649.50006103515625, 451.437139987945557 ],
					"order" : 7,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-81", 0 ],
					"midpoints" : [ 789.50007438659668, 918.461538314819336, 927.961626052856445, 918.461538314819336 ],
					"order" : 5,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-83", 0 ],
					"midpoints" : [ 789.50007438659668, 867.461538314819336, 927.961626052856445, 867.461538314819336 ],
					"order" : 6,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-85", 0 ],
					"midpoints" : [ 789.50007438659668, 751.461538314819336, 629.500059127807617, 751.461538314819336 ],
					"order" : 12,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-86", 0 ],
					"midpoints" : [ 789.50007438659668, 712.461538314819336, 929.500087738037109, 712.461538314819336 ],
					"order" : 0,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-9", 0 ],
					"midpoints" : [ 789.50007438659668, 616.461538314819336, 906.615329742431641, 616.461538314819336, 906.615329742431641, 628.461538314819336, 929.500087738037109, 628.461538314819336 ],
					"order" : 1,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-76", 0 ],
					"midpoints" : [ 649.50006103515625, 487.656656980514526, 607.96159553527832, 487.656656980514526 ],
					"order" : 0,
					"source" : [ "obj-79", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-77", 1 ],
					"midpoints" : [ 649.50006103515625, 487.656656980514526, 616.938497304916382, 487.656656980514526, 616.938497304916382, 460.656656980514526, 572.807743072509766, 460.656656980514526 ],
					"order" : 1,
					"source" : [ "obj-79", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-80", 1 ],
					"midpoints" : [ 927.961626052856445, 953.973732709884644, 902.170243263244629, 953.973732709884644, 902.170243263244629, 923.973732709884644, 890.846233367919922, 923.973732709884644 ],
					"source" : [ "obj-81", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-82", 1 ],
					"midpoints" : [ 927.961626052856445, 912.461538314819336, 902.115329742431641, 912.461538314819336, 902.115329742431641, 876.461538314819336, 891.884695053100586, 876.461538314819336 ],
					"source" : [ "obj-83", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-84", 1 ],
					"midpoints" : [ 629.500059127807617, 801.461538314819336, 599.743250855859742, 801.461538314819336, 599.743250855859742, 759.461538314819336, 576.423126220703125, 759.461538314819336 ],
					"source" : [ "obj-85", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-16", 0 ],
					"midpoints" : [ 929.500087738037109, 754.461538314819336, 900.615329742431641, 754.461538314819336, 900.615329742431641, 724.461538314819336, 886.423160552978516, 724.461538314819336 ],
					"order" : 0,
					"source" : [ "obj-86", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-88", 1 ],
					"midpoints" : [ 929.500087738037109, 762.461538314819336, 858.615329742431641, 762.461538314819336, 858.615329742431641, 724.461538314819336, 848.346233367919922, 724.461538314819336 ],
					"order" : 1,
					"source" : [ "obj-86", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-75", 0 ],
					"midpoints" : [ 1886.423255920410156, 487.041319608688354, 1846.423252105712891, 487.041319608688354 ],
					"order" : 0,
					"source" : [ "obj-89", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-78", 1 ],
					"midpoints" : [ 1886.423255920410156, 487.041319608688354, 1854.784911870956421, 487.041319608688354, 1854.784911870956421, 460.041319608688354, 1809.730937957763672, 460.041319608688354 ],
					"order" : 1,
					"source" : [ "obj-89", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 1 ],
					"midpoints" : [ 929.500087738037109, 659.490367740683268, 905.023899793624878, 659.490367740683268, 905.023899793624878, 625.821134498446213, 884.346233367919922, 625.821134498446213 ],
					"source" : [ "obj-9", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-102", 0 ],
					"midpoints" : [ 2027.96173095703125, 520.041319608688354, 1884.884794235229492, 520.041319608688354 ],
					"order" : 8,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-105", 0 ],
					"midpoints" : [ 2027.96173095703125, 574.846200942993164, 2167.96174430847168, 574.846200942993164 ],
					"order" : 2,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-109", 0 ],
					"midpoints" : [ 2027.96173095703125, 525.846200942993164, 2167.96174430847168, 525.846200942993164 ],
					"order" : 3,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-111", 0 ],
					"midpoints" : [ 2027.96173095703125, 571.041319608688354, 1881.807870864868164, 571.041319608688354 ],
					"order" : 9,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-113", 0 ],
					"midpoints" : [ 2027.96173095703125, 474.846200942993164, 2167.96174430847168, 474.846200942993164 ],
					"order" : 4,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-17", 0 ],
					"midpoints" : [ 2027.96173095703125, 615.846200942993164, 2144.46174430847168, 615.846200942993164, 2144.46174430847168, 627.846200942993164, 2167.96174430847168, 627.846200942993164 ],
					"order" : 1,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-21", 0 ],
					"midpoints" : [ 2027.96173095703125, 864.846200942993164, 1866.423254013061523, 864.846200942993164 ],
					"order" : 10,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-23", 0 ],
					"midpoints" : [ 2027.96173095703125, 808.846200942993164, 1866.423254013061523, 808.846200942993164 ],
					"order" : 11,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-57", 0 ],
					"midpoints" : [ 2027.96173095703125, 711.846200942993164, 2167.96174430847168, 711.846200942993164 ],
					"order" : 0,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-59", 0 ],
					"midpoints" : [ 2027.96173095703125, 917.846200942993164, 2166.423282623291016, 917.846200942993164 ],
					"order" : 5,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-61", 0 ],
					"midpoints" : [ 2027.96173095703125, 866.846200942993164, 2166.423282623291016, 866.846200942993164 ],
					"order" : 6,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-63", 0 ],
					"midpoints" : [ 2027.96173095703125, 750.846200942993164, 1866.423254013061523, 750.846200942993164 ],
					"order" : 12,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-89", 0 ],
					"midpoints" : [ 2027.96173095703125, 450.821802616119385, 1886.423255920410156, 450.821802616119385 ],
					"order" : 7,
					"source" : [ "obj-96", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-100", 0 ],
					"source" : [ "obj-99", 0 ]
				}

			}
 ],
		"originid" : "pat-8",
		"dependency_cache" : [ 			{
				"name" : "OSC-route.mxe64",
				"type" : "mx64"
			}
 ],
		"autosave" : 0,
		"boxgroups" : [ 			{
				"boxes" : [ "obj-18", "obj-64", "obj-65", "obj-4", "obj-67", "obj-70", "obj-76", "obj-77", "obj-79" ]
			}
, 			{
				"boxes" : [ "obj-165", "obj-163", "obj-162", "obj-164", "obj-85", "obj-84", "obj-92" ]
			}
, 			{
				"boxes" : [ "obj-90", "obj-8", "obj-68", "obj-71", "obj-3", "obj-66", "obj-69", "obj-5", "obj-9" ]
			}
, 			{
				"boxes" : [ "obj-80", "obj-82", "obj-83", "obj-81", "obj-11" ]
			}
, 			{
				"boxes" : [ "obj-91", "obj-86", "obj-88", "obj-16" ]
			}
, 			{
				"boxes" : [ "obj-54", "obj-57", "obj-56", "obj-10" ]
			}
, 			{
				"boxes" : [ "obj-58", "obj-60", "obj-61", "obj-59", "obj-13" ]
			}
, 			{
				"boxes" : [ "obj-55", "obj-112", "obj-107", "obj-104", "obj-113", "obj-109", "obj-105", "obj-14", "obj-17" ]
			}
, 			{
				"boxes" : [ "obj-21", "obj-23", "obj-22", "obj-20", "obj-63", "obj-62", "obj-53" ]
			}
, 			{
				"boxes" : [ "obj-111", "obj-103", "obj-97", "obj-110", "obj-101", "obj-102", "obj-75", "obj-78", "obj-89" ]
			}
 ]
	}

}
