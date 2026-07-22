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
	fileName := flag.String("file", "", "저장할 파일 경로")
	serverIP := flag.String("ip", "", "서버 IP 주소")
	serverPort := flag.String("port", "", "서버 포트 번호")
	flag.Parse()

	// 인자 검증
	if *fileName == "" || *serverIP == "" || *serverPort == "" {
		fmt.Println("사용법: go run client.go -file <저장할파일명> -ip <IP주소> -port <포트>")
		os.Exit(1)
	}

	err := os.Remove(*fileName)
    if err != nil {
        fmt.Printf("파일 삭제 실패: %v\n", err)
    }

	file, err := os.OpenFile(*fileName, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
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

	fmt.Printf("%s 서버로부터 데이터를 수신하여 %s에 저장 중...\n", address, *fileName)

	// 데이터 수신 및 파일 쓰기 (EOF 발생 시까지 반복)
	written, err := io.Copy(file, conn)
	if err != nil {
		fmt.Printf("데이터 수신 및 저장 중 오류 발생: %v\n", err)
		return
	}

	fmt.Printf("전송 완료. 총 %d 바이트를 저장했습니다.\n", written)
}
