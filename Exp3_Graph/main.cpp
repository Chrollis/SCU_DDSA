#include "pathfinder.hpp"

int main() {
	chr::city_map Beijing;
	
	Beijing.add_place(1, "天安门广场", 116.3975, 39.9087);
	Beijing.add_place(2, "故宫博物院", 116.3970, 39.9175);
	Beijing.add_place(3, "颐和园", 116.2732, 39.9998);
	Beijing.add_place(4, "圆明园", 116.2975, 40.0101);
	Beijing.add_place(5, "北京大学", 116.3105, 39.9920);
	Beijing.add_place(6, "清华大学", 116.3275, 40.0018);
	Beijing.add_place(7, "鸟巢", 116.3960, 39.9923);
	Beijing.add_place(8, "水立方", 116.3915, 39.9915);
	Beijing.add_place(9, "北京西站", 116.3215, 39.8948);
	Beijing.add_place(10, "北京南站", 116.3785, 39.8652);
	Beijing.add_place(11, "首都机场", 116.5872, 40.0815);
	Beijing.add_place(12, "南苑机场", 116.3880, 39.7828);
	Beijing.add_place(13, "中关村", 116.3123, 39.9832);
	Beijing.add_place(14, "国贸CBD", 116.4595, 39.9095);
	Beijing.add_place(15, "西单商业区", 116.3740, 39.9130);
	Beijing.add_place(16, "王府井大街", 116.4170, 39.9085);
	Beijing.add_place(17, "天坛公园", 116.4070, 39.8820);
	Beijing.add_place(18, "北海公园", 116.3910, 39.9255);
	Beijing.add_place(19, "什刹海", 116.3865, 39.9385);
	Beijing.add_place(20, "八达岭长城", 116.0240, 40.3535);
	Beijing.add_biroad(11, 7);
	Beijing.add_biroad(7, 8);
	Beijing.add_biroad(8, 1);
	Beijing.add_biroad(1, 16);
	Beijing.add_biroad(1, 2);
	Beijing.add_road(1, 14);
	Beijing.add_road(16, 15);
	Beijing.add_road(15, 9);
	Beijing.add_road(2, 18);
	Beijing.add_road(18, 19);
	Beijing.add_road(9, 13);
	Beijing.add_road(13, 14);
	Beijing.add_road(14, 10);
	Beijing.add_road(10, 17);
	Beijing.add_road(17, 12);
	Beijing.add_road(3, 4);
	Beijing.add_road(4, 6);
	Beijing.add_road(6, 20);
	Beijing.add_road(6, 5);
	Beijing.add_road(5, 13);
	while (1) {
		unsigned a, b;
		std::cin >> a >> b;
		try {
			auto path = Beijing.find_path(a, b);
			if (path.empty()) {
				std::cout << "无法抵达\n";
			}
			else {
				Beijing.print_path(path);
			}
		}
		catch (std::invalid_argument i) {
			std::cout << i.what() << std::endl;
		}
	}
	return 0;
}