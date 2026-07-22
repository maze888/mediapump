package main

import (
	"flag"
	"fmt"
	"io"
	"net"
	"os"
)

func main() {
	// 명령행 인자 정의
	filePath := flag.String("file", "", "전송할 파일 경로")
	serverIP := flag.String("ip", "", "서버 IP 주소")
	serverPort := flag.String("port", "", "서버 포트 번호")
	flag.Parse()

	// 인자 검증
	if *filePath == "" || *serverIP == "" || *serverPort == "" {
		fmt.Println("사용법: go run client.go -file <파일경로> -ip <IP주소> -port <포트>")
		os.Exit(1)
	}

	// 파일 열기
	file, err := os.Open(*filePath)
	if err != nil {
		fmt.Printf("파일 열기 실패: %v\n", err)
		return
	}
	defer file.Close()

	// 서버 연결
	address := net.JoinHostPort(*serverIP, *serverPort)
	conn, err := net.Dial("tcp", address)
	if err != nil {
		fmt.Printf("서버 연결 실패: %v\n", err)
		return
	}
	defer conn.Close()

	// 파일 데이터 전송 (스트리밍 방식)
	fmt.Printf("%s 파일을 %s로 전송 중...\n", *filePath, address)
	written, err := io.Copy(conn, file)
	if err != nil {
		fmt.Printf("데이터 전송 중 오류 발생: %v\n", err)
		return
	}

	fmt.Printf("성공적으로 %d 바이트를 전송했습니다.\n", written)
}
