# http_client.py

import requests
import xml.etree.ElementTree as ET

class HTTPClient:
    def get(self, url, params=None, headers=None):
        """
        发送HTTP GET请求

        :param url: 请求的URL
        :param params: URL查询参数（可选）
        :param headers: 请求头（可选）
        :return: 响应内容（XML的ElementTree对象）或None（如果请求失败）
        """
        try:
            response = requests.get(url, params=params, headers=headers)
            response.raise_for_status()  # 如果响应状态码表示错误，则抛出HTTPError异常
            # 解析XML响应
            xml_data = ET.fromstring(response.content)
            return xml_data
        except requests.exceptions.RequestException as e:
            print(f"请求失败: {e}")
            return None
