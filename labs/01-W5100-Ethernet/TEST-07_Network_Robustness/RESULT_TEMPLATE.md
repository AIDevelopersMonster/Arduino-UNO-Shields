# TEST-07 — фактический результат (заполнить после испытаний)

Статус: **PENDING**. Этот файл не является доказательством аппаратного PASS.

- Дата / оператор / обозначение UNO и W5100:
- Commit / firmware version / SHA256 HEX / вывод успешной загрузки:
- Arduino CLI / AVR core / Ethernet / Windows / PowerShell:
- Размеры Flash / static SRAM / sampled min_free / drift:
- Роутер/коммутатор, топология, DHCP lease/T1/T2, актуальные IP:
- Способ fault и способ/погрешность отметки восстановления:

| Сценарий | Параметры / журнал | Результат | recovery_ms / ошибки |
| --- | --- | --- | --- |
| Baseline | | PENDING | |
| Cable | | PENDING | |
| StartupDhcp | | PENDING | |
| DhcpRenew | | PENDING | rc=2 ещё не наблюдался |
| DhcpOutage | | PENDING | |
| Soak | | PENDING | |
| TcpAbort | | PENDING | |

Приложить build-summary.json, summary.json семи сценариев, serial.log,
events.jsonl, probes.csv и certification.json. Привести все отказы и причины
повторных испытаний. Если один обязательный сценарий отсутствует, FULL PASS
не присваивается. Дополнительные cable/router/IP-change проверки описать отдельно.

Границы результата: один образец, указанные fault, сеть и длительность;
microSD / internet / security / непрерывная работа сутками не проверены.
